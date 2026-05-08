#include "drivers/sfs/sfs.h"

#include "drivers/ata/ata.h"

#include "types.h"

#include "std.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "math/math.h"

static byte sfs_superblock_buf[512] = { 0 };

static sfs_file_sector_t file_sector_buf = { 0 };

byte* sfs_err_description(sfs_error_e err) {
	switch (err) {
		case SFS_OK:
			return "OK";
			break;
		
		case SFS_ERROR_DISK_UNAVAILABLE:
			return "disk unavailable";
			break;
		
		case SFS_ERROR_EMPTY_FILENAME:
			return "empty filename";
			break;
		
		case SFS_ERROR_EMPTY_DATA:
			return "empty data";
			break;
		
		case SFS_ERROR_FILE_EXISTS:
			return "file already exists";
			break;
		
		case SFS_ERROR_FILE_NOT_EXISTS:
			return "file not exists";
			break;
		
		case SFS_ERROR_INVLD_SIG:
			return "invalid signature";
			break;
		
		case SFS_ERROR_NOT_ENGH_MMRY:
			return "not enough memory";
			break;
		
		
		default:
			break;
	}

	return "Unknown";
}

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

	for (size_t i = 1; i < table.sectors_cnt; i++) {
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

sfs_error_e sfs_find_file(byte drive, const byte* filename, sfs_file_label_t** file_label_ptr, bool* _ok) {
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

	uint32 offset = 0;

	for (uint32 i = 0; i < table->labels_cnt; i++) {
		sfs_file_label_t* label = ((byte*)after_tabel + offset);

		byte* name = after_tabel + offset + sizeof(sfs_file_label_t);

		if (strncmp(name, filename, MAX(filename_len, label->name_size)) == 0) {
			if (_ok) *_ok = true;

			if (file_label_ptr) *file_label_ptr = label;

			return SFS_OK;
		}

		offset += sizeof(sfs_file_label_t) + label->name_size;
	}
	
	if (_ok) *_ok = false;

	return SFS_ERROR_FILE_NOT_EXISTS;
}

sfs_error_e sfs_write_file_sector(byte drive, sfs_file_table_t* table, const byte* data, uint32 sector_st, uint32 size) {
	if (!data) {
		return SFS_ERROR_EMPTY_DATA;
	}

	int32 offset = 0;

	uint32 data_sectors_size = MAX(1, align_up(size, sizeof(file_sector_buf.data)) / sizeof(file_sector_buf.data));

	for (uint32 i = 0; i < data_sectors_size; i++) {
		table->sector_map[(sector_st + i) / 8] |= (1 << ((sector_st + i) % 8));
		
		memset(&file_sector_buf, 0, sizeof(file_sector_buf));

		int32 diff = MIN(sizeof(file_sector_buf.data), size - offset);

		if (diff > 0) {
			memcpy(file_sector_buf.data, data + offset, diff);
		}

		file_sector_buf.is_not_end = i < (data_sectors_size - 1);

		// kprintf("file_sector_buf.is_not_end: %i\n\r", file_sector_buf.is_not_end);

		if (file_sector_buf.is_not_end) {
			file_sector_buf.next_sector = i + sector_st + 1;
		}

		ata_write_sector(&file_sector_buf, drive, i + sector_st, 1);

		ata_flush();

		offset += sizeof(file_sector_buf.data);
	}

	return SFS_OK;
}

sfs_error_e sfs_free_file_sector(byte drive, sfs_file_table_t* table, uint32 sector_st, uint32 size) {
	if (!table) {
		return SFS_ERROR_EMPTY_DATA;
	}

	uint32 sectors_size = MAX(1, align_up(size, sizeof(file_sector_buf.data)) / sizeof(file_sector_buf.data));

	for (uint32 i = 0; i < sectors_size; i++) {
		table->sector_map[(sector_st + i) / 8] &= ~(1ULL << ((sector_st + i) % 8));
	}

	return SFS_OK;
}

bool sfs_sector_is_free(sfs_file_table_t* table, uint32 sector) {
	uint32 sector_index = sector / 8;

	uint32 sector_bit_index = sector % 8;

	return (table->sector_map[sector_index] & (1ULL << sector_bit_index)) == 0;
}

sfs_error_e sfs_create_file(byte drive, const byte* filename, const byte* content, uint32 size) {
	if (filename == nullptr) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	uint32 filename_len = strlen(filename);

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

	sfs_error_e error = sfs_find_file(drive, filename, nullptr, &ok);

	if (ok) return SFS_ERROR_FILE_EXISTS;

	uint32 data_sectors_size = MAX(1, align_up(size, sizeof(file_sector_buf.data)) / sizeof(file_sector_buf.data));

	int32 data_start = found_in_bitmap_best_fit(data_sectors_size, table->sector_map, sizeof(table->sector_map));

	if (data_start <= 0) {
		return SFS_ERROR_NOT_ENGH_MMRY;
	}

	for (uint32 i = 0; i < data_sectors_size; i++) {
		table->sector_map[(data_start + i) / 8] |= (1 << ((data_start + i) % 8));
	}

	uint32 label_addr = sizeof(sfs_file_table_t);

	sfs_file_label_t* label = sfs_superblock_buf + label_addr;

	for (uint32 i = 0; i < table->labels_cnt; i++) {
		label = sfs_superblock_buf + label_addr;

		label_addr += sizeof(sfs_file_label_t) + label->name_size;
	}

	label = sfs_superblock_buf + label_addr;

	memset(label, 0, sizeof(sfs_file_label_t) + align_up(filename_len, 4));

	label->name_size = align_up(filename_len, 4);

	label->data_start = data_start;

	memcpy((byte*)label + sizeof(sfs_file_label_t), filename, filename_len);

	int32 offset = 0;

	offset = 0;

	sfs_write_file_sector(drive, table, content, data_start, size);

	table->labels_cnt += 1;

	ata_write_sector(sfs_superblock_buf, drive, 0, 1);

	ata_flush();
	
	return SFS_OK;
}

sfs_error_e sfs_read_file(byte drive, const byte* filename, byte* content, uint32 _offset, uint32 size, uint32* readed) {
	if (readed)
		*readed = 0;

	uint32 filename_len = strlen(filename);
	
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

	sfs_file_label_t* label = nullptr;

	size_t offset = 0;

	sfs_error_e error = sfs_find_file(drive, filename, &label, &ok);

	if (error != SFS_OK)
		return error;

	if (!ok) return SFS_ERROR_FILE_NOT_EXISTS;

	offset = 0;

	bool is_not_end = true;

	size_t sector = label->data_start;

	size_t readed_size = 0;

	while (is_not_end && offset < size) {
		// kprintf("data location sector: %zu\n", sector);
		
		memset(&file_sector_buf, 0, sizeof(file_sector_buf));

		ata_read_sector(&file_sector_buf, drive, sector, 1);

		sector = file_sector_buf.next_sector;

		is_not_end = file_sector_buf.is_not_end;
			
		// kprintf("is not end: %i\n\r", is_not_end);

		ssize_t diff = MIN(sizeof(file_sector_buf.data), size - offset);

		if (diff > 0) {
			if (content) {
				memcpy(content + offset, file_sector_buf.data, diff);
			}

			readed_size += diff;
		}

		else break;

		offset += sizeof(file_sector_buf.data);
	}

	if (readed)
		*readed = readed_size;

	return SFS_OK;
}

sfs_error_e sfs_rename_file(byte drive, const byte* src_file_name, const byte* dest_file_name) {
	if (!src_file_name) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	if (!dest_file_name) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	size_t src_file_name_len = strlen(src_file_name);

	size_t dest_file_name_len = strlen(dest_file_name);

	memset(sfs_superblock_buf, 0, sizeof(sfs_superblock_buf));

	ata_read_sector(sfs_superblock_buf, drive, 0, 1);

	if (strncmp(sfs_superblock_buf, "RDEVSFS", 7) != 0) {
		return SFS_ERROR_INVLD_SIG;
	}

	sfs_file_table_t* table = (sfs_file_table_t*)sfs_superblock_buf;

	bool ok = false;

	sfs_error_e error = sfs_find_file(drive, dest_file_name, nullptr, &ok);

	if (ok) return SFS_ERROR_FILE_EXISTS;

	ok = false;

	byte* after_tabel = sfs_superblock_buf + sizeof(sfs_file_table_t);

	sfs_file_label_t* label = nullptr;

	error = sfs_find_file(drive, src_file_name, &label, &ok);

	if (error != SFS_OK)
		return error;

	if (!ok) return SFS_ERROR_FILE_NOT_EXISTS;

	if (dest_file_name_len <= label->name_size) {
		byte* name = (byte*)label + sizeof(sfs_file_label_t);

		memset(name, 0, label->name_size);

		memcpy(name, dest_file_name, dest_file_name_len);
	}

	// else {
	// 	size_t dest_name_sectors_size = MAX(1, align_up(dest_file_name_len, sizeof(file_sector_buf.data)) / sizeof(file_sector_buf.data));

	// 	ssize_t name_start = found_in_bitmap_best_fit(dest_name_sectors_size, table->sector_map, sizeof(table->sector_map));

	// 	error = sfs_write_file_sector(drive, table, dest_file_name, name_start, dest_file_name_len);

	// 	if (error != SFS_OK)
	// 		return error;

	// 	label->name_start = name_start;
	// }

	ata_write_sector(sfs_superblock_buf, drive, 0, 1);

	ata_flush();

	return SFS_OK;
}

sfs_error_e sfs_copy_file(byte drive, const byte* src_file_name, const byte* dest_file_name) {
	if (!src_file_name) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	if (!dest_file_name) {
		return SFS_ERROR_EMPTY_FILENAME;
	}

	size_t src_file_name_len = strlen(src_file_name);

	size_t dest_file_name_len = strlen(dest_file_name);

	memset(sfs_superblock_buf, 0, sizeof(sfs_superblock_buf));

	ata_read_sector(sfs_superblock_buf, drive, 0, 1);

	if (strncmp(sfs_superblock_buf, "RDEVSFS", 7) != 0) {
		return SFS_ERROR_INVLD_SIG;
	}

	sfs_file_table_t* table = (sfs_file_table_t*)sfs_superblock_buf;

	
}

sfs_error_e sfs_list_files(byte drive, uint32* files_cnt, byte* buf, uint32 max_file_name_len) {
	memset(sfs_superblock_buf, 0, sizeof(sfs_superblock_buf));

	ata_read_sector(sfs_superblock_buf, drive, 0, 1);

	if (strncmp(sfs_superblock_buf, "RDEVSFS", 7) != 0) {
		return SFS_ERROR_INVLD_SIG;
	}

	sfs_file_table_t* table = (sfs_file_table_t*)sfs_superblock_buf;

	byte* after_tabel = sfs_superblock_buf + sizeof(sfs_file_table_t);

	uint32 offset = 0;

	uint32 index = 0;

	for (uint32 i = 0; i < table->labels_cnt; i++) {
		sfs_file_label_t* label = after_tabel + offset;

		if (label->name_size <= 0) continue;

		uint32 name_offset = 0;

		bool is_not_end = true;

		byte* name = (byte*)label + sizeof(sfs_file_label_t);

		kprintf("label->name_size = %zu\n", label->name_size);
			
		memcpy(buf + (index * max_file_name_len) + name_offset, name, label->name_size);

		offset += sizeof(sfs_file_label_t) + label->name_size;

		index++;
	}

	if (files_cnt)
		*files_cnt = table->labels_cnt;

	return SFS_OK;
}
