#ifndef _EMULATOR_OS_SFS_H
#define _EMULATOR_OS_SFS_H

#include "types.h"

typedef struct PACKED sfs_file_label {
	size_t name_len;

	size_t location; size_t size;

	/* name bytes */

} sfs_file_label;

#define SFS_TABLE_SIZE (512 / sizeof(sfs_file_label))

typedef struct PACKED sfs_file_table {
	byte always_r;
	byte always_d;
	byte always_e;
	byte always_v;
	byte always_s;
	byte always_f;
	byte always_s2;

	sfs_file_label labels[SFS_TABLE_SIZE];
} sfs_file_table;

void sfs_format(byte drive);

void sfs_create_file(byte drive, byte* name, byte* content, size_t size);

size_t sfs_read_file(byte drive, byte* file_name, byte* content);

byte* sfs_list_files(byte drive, size_t* files_cnt);

#endif
