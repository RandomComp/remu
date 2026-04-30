#include "drivers/sfs/sfs.h"

#include "drivers/ata/ata.h"

#include "types.h"

#include "std.h"

#include "builtins/string.h"

#include "math/math.h"

void sfs_format(byte drive) {
	bool is_master = drive & 0x20;

	if (is_master) {
		kprintf("Formating master disk to sfs...\n");
	}
	else {
		kprintf("Formating slave disk to sfs...\n");
	}

	sfs_file_table table = { 0 };

	table.always_r = 'R';
	table.always_d = 'D';
	table.always_e = 'E';
	table.always_v = 'V';
	table.always_s = 'S';
	table.always_f = 'F';
	table.always_s2 = 'S';

	memset(table.labels, 0, sizeof(table.labels));

	ata_write_sector((uint16*)(&table), drive, 0, 1);

	ata_flush();

	uint32 sectors = 0;

	ata_read_info(drive, &sectors, nullptr, nullptr);

	for (size_t i = 1; i < sectors; i++) {
		ata_write_sector(nullptr, drive, i, 1);

		ata_flush();
	}
	
	kprintf("Formated!\n");
}

bool validate_sfs(byte drive) {
	byte sector[512] = { 0 };

	ata_read_sector(sector, drive, 0, 1);

	return strncmp(sector, "RDEVSFS", 4) == 0;
}

void sfs_create_file(byte drive, byte* name, byte* content, size_t size) {
	kprintf("sfs creating file...\n");

	byte data[512] = { 0 };

	ata_read_sector(data, drive, 0, 1);

	if (strncmp(data, "RDEVSFS", 7) != 0) {
		kprintf("%vfbrsfs cannot create file: invalid sfs signature\n");
		sfs_format(ATA_MASTER);
		return;
	}

	sfs_file_table* table = data;

	size_t label_index = 0; bool ok = false;

	for (size_t i = 0; i < sizeof(table->labels); i++) {
		sfs_file_label* label = &(table->labels[i]);

		if (label->location == 0 ||
			label->size == 0) {
			label_index = i; ok = true; break;
		}
	}

	if (!ok) {
		kprintf("%vfbrsfs cannot create file: no enough space\n"); return;
	}
	
	sfs_file_label* prev_label = &(table->labels[MIN(0, label_index - 1)]);
	
	sfs_file_label* cur_label = &(table->labels[label_index]);

	size_t last_end = prev_label->location + prev_label->name_len + prev_label->size;

	if (last_end == 0) last_end = 512;

	cur_label->location = last_end + 1;
	cur_label->name_len = strlen(name);
	cur_label->size = size;

	size_t file_sector_index = cur_label->location / 512;
	size_t file_sector_byte = cur_label->location % 512;

	byte file_sector[512] = { 0 };

	ata_read_sector(file_sector, drive, file_sector_index, 1);

	memcpy(file_sector + file_sector_byte, name, cur_label->name_len);

	memcpy(file_sector + file_sector_byte + cur_label->name_len, content, size);

	ata_write_sector(data, drive, 0, 1);

	ata_flush();

	ata_write_sector(file_sector, drive, file_sector_index, 1);

	ata_flush();

	kprintf("%vfbgsfs created file!\n");
}

size_t sfs_read_file(byte drive, byte* file_name, byte* content) {
	kprintf("sfs reading file \"%s\"...\n", file_name);

	size_t file_name_len = strlen(file_name);

	byte data[512] = { 0 };

	ata_read_sector(data, drive, 0, 1);

	if (strncmp(data, "RDEVSFS", 7) != 0) {
		kprintf("%vfbrsfs cannot create file: invalid sfs signature\n");
		sfs_format(ATA_MASTER);

		return 0;
	}

	sfs_file_table* table = data;

	size_t label_index = 0; bool ok = false;

	byte temp[512] = { 0 };

	sfs_file_label* label = nullptr;

	size_t file_sector_byte = 0;

	for (size_t i = 0; i < sizeof(table->labels); i++) {
		label = &(table->labels[i]);

		if (label->name_len != file_name_len) continue;

		size_t file_sector_index = label->location / 512;
		file_sector_byte = label->location % 512;

		ata_read_sector(temp, drive, file_sector_index, 1);

		byte* name = temp + file_sector_byte;

		if (strncmp(name, file_name, file_name_len) == 0) {
			label_index = i; ok = true; break;
		}
	}

	if (!ok || !label) {
		kprintf("%vfbrNo such file \"%s\".\n", file_name); return 0;
	}

	if (content) {
		memcpy(content, temp + file_sector_byte + label->name_len, label->size);
	}

	kprintf("%vfbgsfs file \"%s\" readed!\n", file_name);

	return label->size;
}

static byte buf[SFS_TABLE_SIZE * 64] = { { 0 } };

byte* sfs_list_files(byte drive, size_t* files_cnt) {
	byte temp[512] = { 0 };

	ata_read_sector(temp, drive, 0, 1);

	if (strncmp(temp, "RDEVSFS", 7) != 0) {
		kprintf("%vfbrsfs cannot list files: invalid sfs signature\n");
		sfs_format(ATA_MASTER);

		return 0;
	}

	sfs_file_table* table = temp;

	for (size_t i = 0; i < sizeof(table->labels); i++) {
		sfs_file_label* label = &(table->labels[i]);
		
		byte temp[512] = { 0 };

		size_t file_sector_index = label->location / 512;
		size_t file_sector_byte = label->location % 512;

		ata_read_sector(temp, drive, file_sector_index, 1);

		byte* name = temp + file_sector_byte;

		for (size_t j = 0; j < label->name_len; j++) {
			buf[j + (i * 64)] = name[j];
		}

		if (label->location == 0 || label->size == 0) {
			if (files_cnt) *files_cnt = i;
			
			break;
		}
	}

	return buf;
}
