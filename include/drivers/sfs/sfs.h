#ifndef _EMULATOR_OS_SFS_H
#define _EMULATOR_OS_SFS_H

#include "types.h"

#define SFS_MAX_SECTORS 64

typedef struct PACKED sfs_file_label_t {
	size_t name_start;

	size_t data_start;
} sfs_file_label_t;

typedef struct PACKED sfs_file_sector_t {
	byte data[512 - sizeof(size_t) - sizeof(bool)];

	size_t next_sector; bool is_not_end;
} sfs_file_sector_t;

typedef struct PACKED sfs_file_table_t {
	byte always_rdevsfs[7]; uint16 version;

	size_t sectors_cnt; byte sector_map[SFS_MAX_SECTORS / 8];

	size_t labels_cnt;

	/* labels */
} sfs_file_table_t;

typedef enum sfs_error_e {
	SFS_OK,
	SFS_ERROR_DISK_UNAVAILABLE,
	SFS_ERROR_EMPTY_FILENAME,
	SFS_ERROR_FILE_EXISTS,
	SFS_ERROR_FILE_NOT_EXISTS,
	SFS_ERROR_INVLD_SIG,
	SFS_ERROR_NOT_ENGH_MMRY,
} sfs_error_e;

sfs_error_e sfs_format(byte drive);

sfs_error_e sfs_create_file(byte drive, const byte* name, const byte* content, size_t size);

sfs_error_e sfs_read_file(byte drive, const byte* file_name, byte* content, size_t size, size_t* readed);

byte* sfs_list_files(byte drive, size_t* files_cnt);

#endif
