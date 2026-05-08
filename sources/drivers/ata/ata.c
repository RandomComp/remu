#include "drivers/ata/ata.h"

#include "types.h"

#include "std.h"

#include "drivers/io.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "math/math.h"

static const byte errors[] = {
	ATA_ERR_BADBLK,
	ATA_ERR_UNC_DATA,
	ATA_ERR_MEDIA_CHANGED,
	ATA_ERR_ID_NOT_FOUND,
	ATA_ERR_MEDIA_CHANGED_RQ,
	ATA_ERR_ABORT,
	ATA_ERR_TRACK_0_NOT_FOUND,
	ATA_ERR_ADDRESS_MARK_NOT_FOUND,
};

static const byte* errors_descriptions[] = {
	"bad block",
	"uncorrected data",
	"media changed",
	"sector not found",
	"media change request",
	"operation not supported",
	"cannot found 0 track",
	"address mark not found",
};

void ata_wait_until_state(byte _state) {
	byte state = in8(ATA_STATUS);

	while ((state & _state) != _state) {
		state = in8(ATA_STATUS);

		if ((state & ATA_SR_ERR) != 0) {
			kprintf("%vfbrATA error: %s\n", ata_err_description(state));

			kprintf("%vfbrATA status: %b\n", state);

			return;
		}

		#ifdef __EMULATOR__
		halt();
		#endif
	}
}

void ata_wait_not_until_state(byte _state) {
	byte state = in8(ATA_STATUS);

	while ((state & _state) == _state) {
		state = in8(ATA_STATUS);

		if ((state & ATA_SR_ERR) != 0) {
			kprintf("%vfbrATA error: %s\n", ata_err_description(state));

			kprintf("%vfbrATA status: %b\n", state);

			return;
		}

		#ifdef __EMULATOR__
		halt();
		#endif
	}
}

void ata_write_sector(void* _buf, byte drive, uint32 sector, byte sectors) {
	ata_wait_not_until_state(ATA_SR_BSY);

	out8(ATA_DRIVE_SEL, drive);
	out8(ATA_SECTOR_COUNT, sectors);

	out8(ATA_LBA_LOW, sector & 0xFF);
	out8(ATA_LBA_MID, (sector >> 8) & 0xFF);
	out8(ATA_LBA_HIGH, (sector >> 16) & 0xFF);

	out8(ATA_COMMAND, ATA_CMD_WRITE);

	ata_wait_not_until_state(ATA_SR_BSY);

	ata_wait_until_state(ATA_SR_DRQ);

	uint16* buf = (uint16*)_buf;

	for (size_t i = 0; i < 256; i++) {
		if (buf)
			out16(ATA_DATA, buf[i]);
		else
			out16(ATA_DATA, 0x00);
	}
}

void ata_read_sector(void* _buf, byte drive, uint32 sector, byte sectors) {
	ata_wait_not_until_state(ATA_SR_BSY);

	out8(ATA_DRIVE_SEL, drive);
	out8(ATA_SECTOR_COUNT, sectors);

	out8(ATA_LBA_LOW, sector & 0xFF);
	out8(ATA_LBA_MID, (sector >> 8) & 0xFF);
	out8(ATA_LBA_HIGH, (sector >> 16) & 0xFF);

	out8(ATA_COMMAND, ATA_CMD_READ);

	ata_wait_not_until_state(ATA_SR_BSY);

	ata_wait_until_state(ATA_SR_DRQ);

	uint16* buf = (uint16*)_buf;

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

byte ata_read_status(void) {
	return in8(ATA_STATUS);
}

byte* ata_err_description(byte status) {
	if (status == 0xFF)
		return "No disk.";

	if ((status & ATA_SR_ERR) != 0) {
		byte errcode = in8(ATA_ERROR_REG);

		c_str error_msg = "Unknown error.";

		for (size_t i = 0; i < sizeof(errors); i++) {
			if ((errcode & errors[i]) != 0) {
				error_msg = errors_descriptions[i]; break;
			}
		}

		return error_msg;
	}

	return "No error.";
}

bool ata_available(byte drive) {
	return true;
}

void ata_read_info(byte drive, uint32* _sectors, byte* model_name, byte* serial_number) {
	ata_wait_not_until_state(ATA_SR_BSY);

	uint16 info[256] = { 0 };

	out8(ATA_DRIVE_SEL, 0xA0);

	out8(ATA_LBA_LOW, 0);
	out8(ATA_LBA_MID, 0);
	out8(ATA_LBA_HIGH, 0);

	ata_read(info, true);

	if (_sectors) {
		uint32 sectors = 0;

		if ((info[83] & 0b10000000000) == 0) {
			sectors |= (uint32)info[60];

			sectors |= ((uint32)info[61]) << 16;
		}

		*_sectors = sectors;
	}

	if (model_name) {
		for (size_t i = 0; i < 20 && info[i + 27]; i++) {
			uint16 word = info[i + 27];
			
			model_name[(i * 2)] = (word >> 8) & 0xFF;
			
			model_name[(i * 2) + 1] = word & 0xFF;
		}

		strip_str(model_name, 40);
	}

	if (serial_number) {
		for (size_t i = 0; i < 10 && info[i + 10]; i++) {
			uint16 word = info[i + 10];
			
			serial_number[(i * 2)] = (word >> 8) & 0xFF;
			
			serial_number[(i * 2) + 1] = word & 0xFF;
		}

		strip_str(serial_number, 20);
	}
}

uint64 ata_read_size(byte drive) {
	uint32 sectors = 0;

	ata_read_info(drive, &sectors, nullptr, nullptr);

	return (uint64)sectors * 512;
}
