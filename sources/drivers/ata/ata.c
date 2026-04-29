#include "drivers/ata/ata.h"

#include "types.h"

#include "drivers/memory/memory.h"

#include "builtins/builtins.h"

void ata_wait_until_state(byte _state) {
	byte state = in8(ATA_STATUS);

	while ((state & _state) != _state) {
		state = in8(ATA_STATUS);

		#ifdef __EMULATOR__
		halt();
		#endif
	}
}

void ata_wait_not_until_state(byte _state) {
	byte state = in8(ATA_STATUS);

	while ((state & _state) == _state) {
		state = in8(ATA_STATUS);

		#ifdef __EMULATOR__
		halt();
		#endif
	}
}

void ata_write_sector(uint16* _buf, byte drive, uint32 sector, byte sectors) {
	ata_wait_not_until_state(ATA_SR_BSY);

	out8(ATA_DRIVE_SEL, drive);
	out8(ATA_SECTOR_COUNT, sectors);

	out8(ATA_LBA_LOW, sector & 0xFF);
	out8(ATA_LBA_MID, (sector >> 8) & 0xFF);
	out8(ATA_LBA_HIGH, (sector >> 16) & 0xFF);

	out8(ATA_COMMAND, ATA_CMD_WRITE);

	ata_wait_not_until_state(ATA_SR_BSY);

	ata_wait_until_state(ATA_SR_DRQ);

	uint16* buf = _buf;

	for (size_t i = 0; i < 256; i++) {
		out16(ATA_DATA, buf[i]);
	}
}

void ata_read_sector(uint16* buf, byte drive, uint32 sector, byte sectors) {
	ata_wait_not_until_state(ATA_SR_BSY);

	out8(ATA_DRIVE_SEL, drive);
	out8(ATA_SECTOR_COUNT, sectors);

	out8(ATA_LBA_LOW, sector & 0xFF);
	out8(ATA_LBA_MID, (sector >> 8) & 0xFF);
	out8(ATA_LBA_HIGH, (sector >> 16) & 0xFF);

	out8(ATA_COMMAND, ATA_CMD_READ);

	ata_wait_not_until_state(ATA_SR_BSY);

	ata_wait_until_state(ATA_SR_DRQ);

	for (size_t i = 0; i < (size_t)sectors * 256; i++) {
		buf[i] = in16(ATA_DATA);
	}
}

void ata_read(uint16* buf, bool _info) {
	ata_wait_not_until_state(ATA_SR_BSY);

	out8(ATA_COMMAND, _info ? ATA_CMD_INFO : ATA_CMD_READ);

	ata_wait_not_until_state(ATA_SR_BSY);

	ata_wait_until_state(ATA_SR_DRQ);

	for (size_t i = 0; i < 256; i++) {
		buf[i] = in16(ATA_DATA);
	}
}

void ata_flush() {
	out8(ATA_COMMAND, ATA_CMD_FLUSH);

	ata_wait_not_until_state(ATA_SR_BSY);
}

void ata_read_info(byte drive, uint32* _sectors, byte* model_name, byte* serial_number) {
	ata_wait_not_until_state(ATA_SR_BSY);

	uint16 info[256] = { 0 };

	out8(ATA_DRIVE_SEL, drive);

	out8(ATA_LBA_LOW, 0);
	out8(ATA_LBA_MID, 0);
	out8(ATA_LBA_HIGH, 0);

	ata_read(info, true);

	if (_sectors) {
		uint32 sectors = 0;

		if ((info[83] & 0b10000000000) == 0) {
			sectors |= info[60];

			sectors |= info[61] << 16;
		}

		*_sectors = sectors;
	}

	if (model_name) {
		for (size_t i = 0; i < 20 && info[i + 27]; i++) {
			uint16 word = info[i + 27];
			
			model_name[i * 2] = (word >> 8) & 0xFF;
			
			model_name[(i * 2) + 1] = word & 0xFF;
		}
	}

	if (serial_number) {
		for (size_t i = 0; i < 10 && info[i + 10]; i++) {
			uint16 word = info[i + 10];
			
			serial_number[i * 2] = (word >> 8) & 0xFF;
			
			serial_number[(i * 2) + 1] = word & 0xFF;
		}
	}
}

uint64 ata_read_size(byte drive) {
	uint32 sectors = 0;

	ata_read_info(drive, &sectors, nullptr, nullptr);

	return (uint64)sectors * 512;
}

static uint8 sector_buf[512] = { 0 };

static uint64 cur_sector = 0;

static byte cur_drive = 0;

byte ata_read_byte(byte drive, uint64 byte) {
	uint64 sector = byte / 512;

	if (cur_sector != sector || 
		cur_drive != drive) {
		ata_read_sector(sector_buf, ATA_MASTER, (uint32)sector, 1);
	}

	cur_sector = sector;

	cur_drive = drive;

	return sector_buf[byte % 512];
}
