#ifndef _EMULATOR_ATA_PIO_H
#define _EMULATOR_ATA_PIO_H

#include "types.h"

#include <bits/types/FILE.h>

#define ATA_DATA			0x1F0
#define ATA_ERROR_REG		0x1F1
#define ATA_SECTOR_COUNT	0x1F2

#define ATA_LBA_LOW			0x1F3
#define ATA_LBA_MID			0x1F4
#define ATA_LBA_HIGH		0x1F5

#define ATA_DRIVE_SEL		0x1F6

#define ATA_COMMAND			0x1F7
#define ATA_STATUS			0x1F7

#define ATA_CMD_WRITE		0x30
#define ATA_CMD_READ		0x20
#define ATA_CMD_FLUSH		0xE7
#define ATA_CMD_INFO		0xEC

#define ATA_MASTER			0xE0
#define ATA_SR_ERR			0x01
#define ATA_SR_BSY			0x80
#define ATA_SR_DRQ			0x08

#define ATA_INT				0x2E

#define ATA_ERR_BADBLK						0x80
#define ATA_ERR_UNC_DATA					0x40
#define ATA_ERR_MEDIA_CHANGED				0x20
#define ATA_ERR_ID_NOT_FOUND				0x10
#define ATA_ERR_MEDIA_CHANGED_RQ			0x08
#define ATA_ERR_ABORT						0x04
#define ATA_ERR_TRACK_0_NOT_FOUND			0x02
#define ATA_ERR_ADDRESS_MARK_NOT_FOUND		0x01

typedef struct hdd_ata_pio_t {
	byte status; byte error;

	_size_t sectors;

	uint16 sector_buffer[256];

	FILE* master_file;
} hdd_ata_pio_t;

hdd_ata_pio_t* init_hdd_ata_pio(_size_t sectors);

void free_hdd_ata_pio(hdd_ata_pio_t* hdd);

#endif
