#include "ata/emulator_hdd_pio.h"

#include "emulator_logger.h"

#include "emulator_io.h"

#include "cpu/emulator_pic.h"

#include "types.h"

#include <malloc.h>

#include <string.h>

#include <errno.h>

#include <bits/types/FILE.h>

static hdd_ata_pio_t* cur = nullptr;

static bool master_selected = false;

static byte sectors_cnt = 0xFF;

static uint32 lba_addr = 0x00000000;

static _size_t ata_word_index = 0, cur_sector = 0;

#define DATE_WITHOUT_YEAR (char[]){__DATE__[0], __DATE__[1], __DATE__[2], __DATE__[3], __DATE__[4], __DATE__[5], '\0'}

static void ata_reset() {
	if (!cur) return;

	cur->status = 0x00000000;

	cur->error = 0x00000000;

	cur_sector = 0;

	memset(cur->sector_buffer, 0, 512);
}

static _size_t read_ata_state() {
	if (!cur) return 0xFFFFFFFFFFFFFFFF;
	
	emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO reading state %b", cur->status);

	return cur->status;
}

static void select_drive(_size_t data) {
	if (!cur) return;

	data = data & 0xFF;

	// CHS and floppy is now unsupported

	bool is_master = data & 0x20;

	if (!is_master) return;

	master_selected = is_master;

	if (master_selected) {
		emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO master selected");
	}
	else {
		emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO slave selected");
	}
}

static void write_sectors_cnt(_size_t data) {
	if (!cur) return;

	data = data & 0xFF;

	sectors_cnt = data;

	emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO sectors cnt %llu", data);
}

static void write_lba_low(_size_t data) {
	if (!cur) return;

	data = data & 0xFF;

	emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO writing lba low address %x", data);

	lba_addr &= ~0x0000FF;

	lba_addr |= data;
}

static void write_lba_mid(_size_t data) {
	if (!cur) return;

	data = data & 0xFF;

	emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO writing lba middle address %x", data);

	lba_addr &= ~0x00FF00;

	lba_addr |= (data << 8);
}

static void write_lba_high(_size_t data) {
	if (!cur) return;

	data = data & 0xFF;

	emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO writing lba high address %x", data);

	lba_addr &= ~0xFF0000;

	lba_addr |= (data << 16);
}

static void ata_command(_size_t command) {
	if (!cur) return;

	command = command & 0xFF;

	ata_word_index = 0;

	cur->status = 0;

	cur->error = 0;

	// memset(cur->sector_buffer, 0, 512);

	if (command == ATA_CMD_READ) {
		cur->status |= ATA_SR_BSY;
		
		emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO read command from address %x", lba_addr);
		
		if (lba_addr >= cur->sectors) {
			cur->status |= ATA_SR_ERR;

			cur->error |= ATA_ERR_ID_NOT_FOUND;

			emulator_log(false, LOG_SEVERITY_ERROR, "HDD ATA PIO read error: addr %u excceds %u sectors", lba_addr, cur->sectors);

			cur->status &= ~ATA_SR_BSY;

			return;
		}

		memset(cur->sector_buffer, 0, sizeof(cur->sector_buffer));

		fseek(cur->master_file, (long)lba_addr * 512, SEEK_SET);

		fread(cur->sector_buffer, 512, 1, cur->master_file);

		cur_sector = 0;
		
		cur->status &= ~ATA_SR_BSY;

		cur->status |= ATA_SR_DRQ;
	}

	else if (command == ATA_CMD_WRITE) {
		emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO write commmand to address %x", lba_addr);

		if (lba_addr >= cur->sectors) {
			cur->status |= ATA_SR_ERR;

			cur->error |= ATA_ERR_ID_NOT_FOUND;

			emulator_log(false, LOG_SEVERITY_ERROR, "HDD ATA PIO write error: addr %u excceds %u sectors", lba_addr, cur->sectors);

			cur->status &= ~ATA_SR_BSY;
			
			return;
		}
		
		cur_sector = 0;

		cur->status &= ~ATA_SR_BSY;
			
		cur->status |= ATA_SR_DRQ;
	}

	else if (command == ATA_CMD_FLUSH) {
		emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO buffer flushing...");

		if (lba_addr >= cur->sectors) {
			cur->error |= ATA_ERR_ID_NOT_FOUND;

			lba_addr = 0;
			
			return;
		}
		
		cur->status |= ATA_SR_BSY;

		fseek(cur->master_file, (long)lba_addr * 512, SEEK_SET);

		fwrite(cur->sector_buffer, 512, 1, cur->master_file);
		
		memset(cur->sector_buffer, 0, sizeof(cur->sector_buffer));
		
		cur->status &= ~ATA_SR_BSY;

		emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO buffer flushed!");
	}

	else if (command == ATA_CMD_INFO) {
		cur->status |= ATA_SR_BSY;

		uint16 buf[256] = { 0 };
		
		buf[83] &= ~0b10000000000; // LBA48 support bit disabling

		if (master_selected) {
			buf[60] = cur->sectors;

			buf[61] = (cur->sectors) >> 16;
		}

		_size_t kb = cur->sectors / 2;

		byte temp_buf[40] = { 0 };

		int writed_cnt = snprintf(temp_buf, 20, "EMUA3200%.3zuKB", kb);

		for (_size_t i = 0; i < 20; i++) {
			buf[27 + i] = temp_buf[(i * 2) + 1] & 0xFF;

			buf[27 + i] |= (temp_buf[i * 2] & 0xFF) << 8;
		}

		memset(temp_buf, 0, 20);
		
		snprintf(temp_buf, 20, "%.3zuKB", kb);

		for (_size_t i = 0; i < 10; i++) {
			buf[10 + i] = temp_buf[(i * 2) + 1] & 0xFF;

			buf[10 + i] |= (temp_buf[i * 2] & 0xFF) << 8;
		}

		memcpy(cur->sector_buffer, buf, 512);

		cur->status &= ~ATA_SR_BSY;

		cur->status |= ATA_SR_DRQ;
	}

	call_emulator_int(nullptr, ATA_INT);
}

static void write_ata_word(_size_t data) {
	if (!cur) return;

	if (ata_word_index >= 256) {
		ata_word_index = 0;

		if (cur_sector < sectors_cnt) {
			cur->status |= ATA_SR_BSY;

			if (lba_addr >= cur->sectors) {
				cur->status |= ATA_SR_ERR;

				cur->error |= ATA_ERR_ID_NOT_FOUND;

				emulator_log(false, LOG_SEVERITY_ERROR, "HDD ATA PIO write error: addr %x excceds sectors %x", lba_addr, cur->sectors);

				cur->status &= ~ATA_SR_BSY;

				lba_addr = 0;
				
				return;
			}

			fseek(cur->master_file, (long)lba_addr * 512, SEEK_SET);

			fwrite(cur->sector_buffer, 512, 1, cur->master_file);

			cur->status &= ~ATA_SR_BSY;

			cur_sector += 1;
		}
		
		else {
			cur->status = 0; cur_sector = 0;

			return;
		}
	}

	data = data & 0xFFFF;

	cur->sector_buffer[ata_word_index] = data;

	ata_word_index += 1;

	emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO writed \"0x%x%x\" to buffer", (data >> 8) & 0xFF, data & 0xFF);
}

static _size_t read_ata_word() {
	if (!cur) return 0xFF;

	if (ata_word_index >= 256) {
		ata_word_index = 0;

		if (cur_sector < sectors_cnt) {
			cur->status |= ATA_SR_BSY;

			cur->status &= ~ATA_SR_DRQ;

			if (lba_addr >= cur->sectors) {
				cur->status |= ATA_SR_ERR;

				cur->error |= ATA_ERR_ID_NOT_FOUND;

				emulator_log(false, LOG_SEVERITY_ERROR, "HDD ATA PIO read error: addr %x excceds sectors %x", lba_addr, cur->sectors);

				cur->status &= ~ATA_SR_BSY;

				lba_addr = 0;
				
				return 0xFF;
			}

			fseek(cur->master_file, (long)lba_addr * 512, SEEK_SET);

			fread(cur->sector_buffer, 512, 1, cur->master_file);

			cur->status &= ~ATA_SR_BSY;

			cur->status |= ATA_SR_DRQ;

			cur_sector += 1;
		}
		
		else {
			cur->status = 0x00000000;

			cur->error = 0x00000000;

			cur_sector = 0;

			return 0;
		}
	}

	uint16 result = cur->sector_buffer[ata_word_index];

	cur->sector_buffer[ata_word_index] = 0xFFFF;

	ata_word_index += 1;

	emulator_log(false, LOG_SEVERITY_TRACE, "HDD ATA PIO readed \"0x%.2x%.2x\" from buffer", (result >> 8) & 0xFF, result & 0xFF);

	return (_size_t)(result & 0xFFFF);
}

_size_t read_error_reg() {
	if (!cur) return ATA_ERR_ABORT;

	return cur->error;
}

hdd_ata_pio_t* init_hdd_ata_pio(_size_t sectors) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "ATA PIO initializing...");

	emulator_log(false, LOG_SEVERITY_VERBOSE, "ATA PIO setting up ports (ATA_STATUS, ATA_COMMAND, ATA_DRIVE_SEL, ATA_SECTOR_COUNT, ATA_LBA_LOW, ATA_LBA_MID, ATA_LBA_HIGH, ATA_DATA)");

	emulator_setup_port_in(ATA_ERROR_REG, read_error_reg);

	emulator_setup_port_in(ATA_STATUS, read_ata_state);
	emulator_setup_port_out(ATA_COMMAND, ata_command);

	emulator_setup_port_out(ATA_DRIVE_SEL, select_drive);
	emulator_setup_port_out(ATA_SECTOR_COUNT, write_sectors_cnt);

	emulator_setup_port_out(ATA_LBA_LOW, write_lba_low);
	emulator_setup_port_out(ATA_LBA_MID, write_lba_mid);
	emulator_setup_port_out(ATA_LBA_HIGH, write_lba_high);

	emulator_setup_port_out(ATA_DATA, write_ata_word);
	emulator_setup_port_in(ATA_DATA, read_ata_word);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Creating ATA HDD with size %llu kb...", sectors * 512);

	hdd_ata_pio_t* hdd = malloc(sizeof(hdd_ata_pio_t));

	memset(hdd->sector_buffer, 0, sizeof(hdd->sector_buffer));

	hdd->status = 0x00000000;

	hdd->error = 0x00000000;

	hdd->sectors = sectors;

	// hdd->buffer_size = 256;

	// hdd->sector_buffer = malloc(hdd->buffer_size * 2);

	// memset(hdd->sector_buffer, 0, hdd->buffer_size * 2);

	hdd->master_file = fopen("hdd_master.img", "rb+");

	if (!hdd->master_file) {
		hdd->master_file = fopen("hdd_master.img", "wb+");
	}

	if (!hdd->master_file) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot create the \"hdd_master.img\": %s", strerror(errno));

		free_hdd_ata_pio(hdd);

		return nullptr;
	}

	fseek(hdd->master_file, 0, SEEK_END);

	size_t start = ftell(hdd->master_file) / 512;

	for (size_t i = start; i < sectors; i++) {
		fwrite(hdd->sector_buffer, 512, 1, hdd->master_file);
	}

	fflush(hdd->master_file);

	setbuf(hdd->master_file, nullptr);	

	fseek(hdd->master_file, 0, SEEK_SET);

	cur = hdd;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Created ATA HDD with size %llu kb", sectors * 512);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "HDD and ATA PIO initialized");

	return hdd;
}

void free_hdd_ata_pio(hdd_ata_pio_t* hdd) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "HDD ATA PIO deinitializing...");

	if (hdd == cur) cur = nullptr;

	if (hdd->master_file)
		fclose(hdd->master_file);
	
	hdd->master_file = nullptr;

	if (hdd) free(hdd);

	emulator_release_port_in(ATA_ERROR_REG);

	emulator_release_port_in(ATA_STATUS);
	emulator_release_port_out(ATA_COMMAND);

	emulator_release_port_out(ATA_DRIVE_SEL);
	emulator_release_port_out(ATA_SECTOR_COUNT);

	emulator_release_port_out(ATA_LBA_LOW);
	emulator_release_port_out(ATA_LBA_MID);
	emulator_release_port_out(ATA_LBA_HIGH);

	emulator_release_port_out(ATA_DATA);
	emulator_release_port_in(ATA_DATA);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "HDD ATA PIO deinitialized");
}
