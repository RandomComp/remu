#ifndef _EMULATOR_OS_MBR_H
#define _EMULATOR_OS_MBR_H

#include "types.h"

typedef struct PACKED mbr_entry_t {
	byte bootable; // 7 bit -- bootable flag
	byte unused_start_chs[3]; // outdated
	byte partition_type;
	byte unused_end_chs[3]; // outdated

	uint32 start_lba;
	uint32 size_sectors;
} mbr_entry_t;

typedef struct PACKED mbr_sector_t {
	byte  boot_code[446];

	mbr_entry_t parts[4];

	uint16 signature;
} mbr_sector_t;

#endif
