#ifndef _EMULATOR_OS_ATA_H
#define _EMULATOR_OS_ATA_H

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
#define ATA_SR_ANY_ERR		0x21
#define ATA_SR_DRQ			0x08
#define ATA_SR_BSY			0x80

#define ATA_INT				0x2E

#define ATA_ERR_BADBLK						0x80
#define ATA_ERR_UNC_DATA					0x40
#define ATA_ERR_MEDIA_CHANGED				0x20
#define ATA_ERR_ID_NOT_FOUND				0x10
#define ATA_ERR_MEDIA_CHANGED_RQ			0x08
#define ATA_ERR_ABORT						0x04
#define ATA_ERR_TRACK_0_NOT_FOUND			0x02
#define ATA_ERR_ADDRESS_MARK_NOT_FOUND		0x01

#include "types.h"

void ata_wait_until_state(byte state);

void ata_wait_not_until_state(byte state);

void ata_write_sector(void* _buf, byte drive, uint32 sector, byte sectors);

void ata_read_sector(void* _buf, byte drive, uint32 sector, byte sectors);

void ata_read(uint16* _buf, bool _info);

void ata_read_info(byte drive, uint32* _sectors, byte* model_name, byte* serial_number);

void ata_flush();

byte* ata_get_error();

uint64 ata_read_size(byte drive);

byte ata_read_byte(byte drive, uint64 byte);

#endif
