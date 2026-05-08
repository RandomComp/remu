#ifndef _EMULATOR_OS_SFS_H
#define _EMULATOR_OS_SFS_H

#include "types.h"

#define SFS_MAX_SECTORS 64

typedef struct PACKED sfs_file_label_t {
	uint32 data_start;

	uint32 name_size;

	/* name */
} sfs_file_label_t;

typedef struct PACKED sfs_file_sector_t {
	byte data[512 - sizeof(uint32) - sizeof(bool)];

	uint32 next_sector; bool is_not_end;
} sfs_file_sector_t;

typedef struct PACKED sfs_file_table_t {
	byte always_rdevsfs[7]; uint16 version;

	uint32 sectors_cnt; byte sector_map[SFS_MAX_SECTORS / 8];

	uint32 labels_cnt;

	/* labels */
} sfs_file_table_t;

typedef enum sfs_error_e {
	SFS_OK,
	SFS_ERROR_DISK_UNAVAILABLE,
	SFS_ERROR_EMPTY_FILENAME,
	SFS_ERROR_EMPTY_DATA,
	SFS_ERROR_FILE_EXISTS,
	SFS_ERROR_FILE_NOT_EXISTS,
	SFS_ERROR_INVLD_SIG,
	SFS_ERROR_NOT_ENGH_MMRY,
} sfs_error_e;

byte* sfs_err_description(sfs_error_e err);

sfs_error_e sfs_format(byte drive);

sfs_error_e sfs_find_file(byte drive, const byte* filename, sfs_file_label_t** file_label_ptr, bool* _ok);

sfs_error_e sfs_write_file_sector(byte drive, sfs_file_table_t* table, const byte* data, uint32 sector_st, uint32 size);

sfs_error_e sfs_free_file_sector(byte drive, sfs_file_table_t* table, uint32 sector_st, uint32 size);

bool sfs_sector_is_free(sfs_file_table_t* table, uint32 sector);

sfs_error_e sfs_create_file(byte drive, const byte* name, const byte* content, uint32 size);
sfs_error_e sfs_read_file(byte drive, const byte* file_name, byte* content, uint32 offset, uint32 size, uint32* readed);
sfs_error_e sfs_rename_file(byte drive, const byte* src_file_name, const byte* dest_file_name);
sfs_error_e sfs_copy_file(byte drive, const byte* src_file_name, const byte* dest_file_name);
sfs_error_e sfs_list_files(byte drive, uint32* files_cnt, byte* buf, uint32 max_file_name_len);

#endif
