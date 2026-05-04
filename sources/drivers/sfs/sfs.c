#include "drivers/sfs/sfs.h"

#include "drivers/ata/ata.h"

#include "types.h"

#include "std.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "math/math.h"

static byte sfs_superblock_buf[512] = { 0 };

static sfs_file_sector_t file_sector_buf = { 0 };

sfs_error_e sfs_format(byte drive) {
	uint32 sectors = 0;

	ata_read_info(drive, &sectors, nullptr, nullptr);

	sfs_file_table_t table = { 0 };

	memcpy(table.always_rdevsfs, "RDEVSFS", 7);

	table.version = 0x0010;
	table.labels_cnt = 0;
	table.sectors_cnt = MIN(SFS_MAX_SECTORS, sectors);
	memset(table.sector_map, 0, sizeof(table.sector_map));

	table.sector_map[0] = 1;

	memset(sfs_superblock_buf, 0, sizeof(sfs_superblock_buf));

	memcpy(sfs_superblock_buf, &table, sizeof(sfs_file_table_t));

	ata_write_sector(sfs_superblock_buf, drive, 0, 1);

	ata_flush();

	for (size_t i = 1; i < SFS_MAX_SECTORS; i++) {
		ata_write_sector(nullptr, drive, i, 1);

		ata_flush();
	}
	
	return SFS_OK;
}

bool validate_sfs(byte drive) {
	byte sector[512] = { 0 };

	ata_read_sector(sector, drive, 0, 1);

	return strncmp(sector, "RDEVSFS", 8) == 0;
}

static ssize_t found_in_bitmap_fast_fit(byte* bitmap, size_t bitmap_size) {
	size_t start = 0;

	bool ok = false;

	for (size_t i = 0; i < bitmap_size * 8; i++) {
		byte map_byte = bitmap[i / 8];

		if (map_byte >= 0xFF) continue;

		byte sector_map_bit = map_byte & (1 << (i % 8));

		if (!sector_map_bit) {
			ok = true; break;
		}
	}

	return ok ? start : -1;
}

static ssize_t found_in_bitmap_best_fit(size_t size, byte* bitmap, size_t bitmap_size) {
	size_t start = 0; size_t available_size = 0;

	bool ok = false;

	for (size_t i = 0; i < bitmap_size * 8; i++) {
		byte map_byte = bitmap[i / 8];

		if (map_byte >= 0xFF) continue;

		byte sector_map_bit = map_byte & (1 << (i % 8));

		if (!sector_map_bit) {
			if (start == 0)
				start = i;
			
			available_size++;
		}

		else {
			start = 0; available_size = 0;
		}

		if (available_size >= size) {
			ok = true; break;
		}
	}

	return ok ? start : -1;
}

sfs_error_e sfs_find_file(byte drive, const byte* filename, sfs_file_label_t* file_label, bool* _ok) {
	if (filename == nullptr) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	size_t filename_len = strlen(filename);

	if (filename_len == 0) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	memset(sfs_superblock_buf, 0, sizeof(sfs_superblock_buf));

	memset(&file_sector_buf, 0, sizeof(file_sector_buf));

	ata_read_sector(sfs_superblock_buf, drive, 0, 1);

	if (strncmp(sfs_superblock_buf, "RDEVSFS", 7) != 0) {
		return SFS_ERROR_INVLD_SIG;
	}

	sfs_file_table_t* table = (sfs_file_table_t*)sfs_superblock_buf;

	byte* after_tabel = sfs_superblock_buf + sizeof(sfs_file_table_t);

	size_t offset = 0;

	for (size_t i = 0; i < table->labels_cnt; i++) {
		sfs_file_label_t* label = after_tabel + offset;

		size_t name_offset = 0;

		bool is_not_end = true;

		uint32 sector = label->name_start;

		while (is_not_end && sector != 0) {
			// kprintf("name sector: %zu\n", sector);
		
			memset(&file_sector_buf, 0, sizeof(sfs_file_sector_t));

			ata_read_sector(&file_sector_buf, drive, sector, 1);

			sector = file_sector_buf.next_sector;

			is_not_end = file_sector_buf.is_not_end;

			size_t name_len = strlen(file_sector_buf.data);

			if (strncmp(file_sector_buf.data, filename, filename_len) == 0) {
				if (_ok) *_ok = true;

				if (file_label) *file_label = *label;

				return SFS_OK;
			}

			name_offset += name_len;
		}

		offset += sizeof(sfs_file_label_t);
	}

	return SFS_OK;
}

sfs_error_e sfs_create_file(byte drive, const byte* filename, const byte* content, size_t size) {
	if (filename == nullptr) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	size_t filename_len = strlen(filename);

	if (filename_len == 0) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	memset(sfs_superblock_buf, 0, sizeof(sfs_superblock_buf));

	memset(&file_sector_buf, 0, sizeof(file_sector_buf));

	ata_read_sector(sfs_superblock_buf, drive, 0, 1);

	if (strncmp(sfs_superblock_buf, "RDEVSFS", 7) != 0) {
		return SFS_ERROR_INVLD_SIG;
	}

	sfs_file_table_t* table = (sfs_file_table_t*)sfs_superblock_buf;

	bool ok = false;

	byte* after_tabel = sfs_superblock_buf + sizeof(sfs_file_table_t);

	sfs_error_e error = sfs_find_file(drive, filename, &file_sector_buf, &ok);

	if (error != SFS_OK)
		return error;

	if (ok) return SFS_ERROR_FILE_EXISTS;

	size_t name_sectors_size = MAX(1, filename_len / 512);

	size_t data_sectors_size = MAX(1, size / 512);

	ssize_t name_start = found_in_bitmap_best_fit(name_sectors_size, table->sector_map, sizeof(table->sector_map));

	if (name_start <= 0) {
		return SFS_ERROR_NOT_ENGH_MMRY;
	}

	for (size_t i = 0; i < name_sectors_size; i++) {
		table->sector_map[(name_start + i) / 8] |= 1 << ((name_start + i) % 8);
	}

	ssize_t data_start = found_in_bitmap_best_fit(data_sectors_size, table->sector_map, sizeof(table->sector_map));

	if (data_start <= 0) {
		return SFS_ERROR_NOT_ENGH_MMRY;
	}

	sfs_file_label_t label = { 0 };

	label.name_start = name_start;
	label.data_start = data_start;

	size_t offset = 0;

	for (size_t i = 0; i < name_sectors_size; i++) {
		memset(&file_sector_buf, 0, sizeof(file_sector_buf));

		memcpy(file_sector_buf.data, filename + offset, sizeof(file_sector_buf.data));

		file_sector_buf.is_not_end = i < (name_sectors_size - 1);

		if (file_sector_buf.is_not_end) {
			file_sector_buf.next_sector = i + name_start + 1;
		}

		ata_write_sector(&file_sector_buf, drive, i + name_start, 1);

		ata_flush();

		offset += sizeof(file_sector_buf.data);
	}

	offset = 0;

	for (size_t i = 0; i < data_sectors_size; i++) {
		table->sector_map[(data_start + i) / 8] |= (1 << ((data_start + i) % 8));
		
		memset(&file_sector_buf, 0, sizeof(file_sector_buf));

		memcpy(file_sector_buf.data, content + offset, sizeof(file_sector_buf.data));

		file_sector_buf.is_not_end = i < (data_sectors_size - 1);

		// kprintf("file_sector_buf.is_not_end: %i\n\r", file_sector_buf.is_not_end);

		if (file_sector_buf.is_not_end) {
			file_sector_buf.next_sector = i + data_start + 1;
		}

		ata_write_sector(&file_sector_buf, drive, i + data_start, 1);

		ata_flush();

		offset += sizeof(file_sector_buf.data);
	}

	size_t label_addr = sizeof(sfs_file_table_t) + sizeof(sfs_file_label_t) * table->labels_cnt;
	
	memcpy(sfs_superblock_buf + label_addr, &label, sizeof(sfs_file_label_t));

	table->labels_cnt += 1;

	ata_write_sector(sfs_superblock_buf, drive, 0, 1);

	ata_flush();
	
	return SFS_OK;
}

sfs_error_e sfs_read_file(byte drive, const byte* filename, byte* content, size_t size, size_t* readed) {
	if (readed)
		*readed = 0;

	size_t filename_len = strlen(filename);
	
	if (filename_len == 0) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	memset(sfs_superblock_buf, 0, sizeof(sfs_superblock_buf));

	ata_read_sector(sfs_superblock_buf, drive, 0, 1);

	if (strncmp(sfs_superblock_buf, "RDEVSFS", 7) != 0) {
		return SFS_ERROR_INVLD_SIG;
	}

	sfs_file_table_t* table = (sfs_file_table_t*)sfs_superblock_buf;

	bool ok = false;

	byte* after_tabel = sfs_superblock_buf + sizeof(sfs_file_table_t);

	sfs_file_label_t* label = (sfs_file_label_t*)after_tabel;

	size_t offset = 0;

	sfs_error_e error = sfs_find_file(drive, filename, label, &ok);

	if (error != SFS_OK)
		return error;

	if (!ok) return SFS_ERROR_FILE_NOT_EXISTS;

	offset = 0;

	bool is_not_end = true;

	size_t sector = label->data_start;

	while (is_not_end) {
		// kprintf("data location sector: %zu\n", sector);
		
		memset(&file_sector_buf, 0, sizeof(file_sector_buf));

		ata_read_sector(&file_sector_buf, drive, sector, 1);

		sector = file_sector_buf.next_sector;

		is_not_end = file_sector_buf.is_not_end;
			
		// kprintf("is not end: %i\n\r", is_not_end);

		memcpy(content + offset, file_sector_buf.data, MIN(size - offset, sizeof(file_sector_buf.data)));

		offset += sizeof(file_sector_buf.data);
	}

	if (readed)
		*readed = offset;

	return SFS_OK;
}

static byte temp_result[50 * 64] = { 0 };

byte* sfs_list_files(byte drive, size_t* files_cnt) {
	memset(sfs_superblock_buf, 0, sizeof(sfs_superblock_buf));

	ata_read_sector(sfs_superblock_buf, drive, 0, 1);

	if (strncmp(sfs_superblock_buf, "RDEVSFS", 7) != 0) {
		kprintf("%vfbrsfs cannot list files: invalid sfs signature\n");

		return 0;
	}
	
	memset(temp_result, 0, 50 * 64);

	sfs_file_table_t* table = (sfs_file_table_t*)sfs_superblock_buf;

	byte* after_tabel = sfs_superblock_buf + sizeof(sfs_file_table_t);

	size_t offset = 0;

	for (size_t i = 0; i < table->labels_cnt; i++) {
		sfs_file_label_t* label = after_tabel + offset;

		size_t name_offset = 0;

		bool is_not_end = true;

		size_t sector = label->name_start;

		while (is_not_end) {
			memset(&file_sector_buf, 0, sizeof(file_sector_buf));

			ata_read_sector(&file_sector_buf, drive, sector, 1);

			sector = file_sector_buf.next_sector;

			is_not_end = file_sector_buf.is_not_end;

			size_t name_len = strlen(file_sector_buf.data);
			
			memcpy(temp_result + (i * 64) + name_offset, file_sector_buf.data, name_len);

			name_offset += name_len;
		}

		offset += sizeof(sfs_file_label_t);
	}

	if (files_cnt)
		*files_cnt = table->labels_cnt;

	return temp_result;
}
