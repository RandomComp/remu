#include "types.h"

#include "std.h"

#include "builtins/string.h"
#include "builtins/builtins.h"

#include "math/math.h"

#include "drivers/ata/ata.h"
#include "drivers/sfs/sfs.h"

#include "drivers/hid/kbdps2.h"

#include "emush/emush.h"

int ata_cmd(const byte** argv, size_t argc) {
	if (argc <= 1) {
		kprintf("%vfbrToo few arguments, needing minimal one argument: action name\n");

		return -1;
	}

	const char* action = argv[1];

	uint32 sectors = 0;

	byte model_name[40] = { 0 };

	byte serial_number[20] = { 0 };

	ata_read_info(ATA_MASTER, &sectors, model_name, serial_number);

	if (strcmp(action, "help") == 0) {
		kprintf("%vfbyUsage:\n");

		kprintf("\t%vfbyata [action] [sector (if dump/read)] [data (if write)]\n");

		kprintf("available actions: \n");

		kprintf("\thelp -- see help\n");

		kprintf("\tinfo -- see ATA info\n");

		kprintf("\tdump -- see ATA sector dump\n");

		kprintf("\twrite -- write data to ATA\n");

		return 0;
	}

	else if (strcmp(action, "info") == 0) {
		size_t mb = sectors / (1024 * 2);

		kprintf("\tSectors count: %vfby%i%vd (%vfby%i%vd mb)\n", sectors, mb);
		
		kprintf("\tModel name: %s\n", model_name);
		
		kprintf("\tSerial number: %s\n", serial_number);

		kprintf("\tLast error: %s\n", ata_get_error());

		return 0;
	}

	else if (strcmp(action, "dump") == 0) {
		byte buf[512] = { 0 };

		size_t sector = 0;

		if (argc >= 3) {
			parse_num(argv[argc - 1], 10, &sector, nullptr);
		}
	
		ata_read_sector(buf, ATA_MASTER, sector, 1);

		ssize_t offset = sector * 512;
		
		clear_screen(0x0F);

		print_hex(buf + (offset % 512), offset, 256);

		// set_cursor_pos(0, ROWS - 1);

		// kprintf("Sector: %zu", sector);

		byte c = getch();

		while (lower(c) != 'q') {
			c = getch();

			if (c == '\x18') {
				clear_screen(0x0F);

				offset = MAX(0, offset - 256);

				if ((offset / 512) != sector) {
					sector = (offset / 512);
					
					ata_read_sector(buf, ATA_MASTER, sector, 1);
				}

				print_hex(buf + (offset % 512), offset, 256);

				// set_cursor_pos(0, ROWS - 1);

				// kprintf("Sector: %zu", sector);
			}

			if (c == '\x19') {
				clear_screen(0x0F);

				offset = MIN(offset + 256, (sectors * 512) - 256);

				if ((offset / 512) != sector) {
					sector = (offset / 512);

					ata_read_sector(buf, ATA_MASTER, sector, 1);
				}

				print_hex(buf + (offset % 512), offset, 256);

				// set_cursor_pos(0, ROWS - 1);

				// kprintf("Sector: %zu", sector);
			}

			halt();
		}

		kprint("\n");

		return 0;
	}

	if (argc <= 2) {
		kprintf("%vfbrToo few arguments, needing 2 argument: action name and sector\n");

		return -1;
	}

	uintmax_t sector = 0;
	
	parse_num(argv[2], 10, &sector, nullptr);

	byte buf[512] = { 0 };
	
	if (strcmp(action, "write") == 0) {
		if (argc < 3) {
			kprintf("%vfbrToo few arguments, needing 3 and more arguments: action name, sector and data1, data2... etc \n");

			return -1;
		}

		uint32 sectors = 0;

		ata_read_info(ATA_MASTER, &sectors, nullptr, nullptr);

		size_t buf_index = 0;

		for (size_t i = 1; i < argc - 2; i++) {
			buf_index += sprintf(buf + buf_index, "%s ", argv[i + 3]);
		}

		ata_write_sector(buf, ATA_MASTER, sector, 1);

		ata_flush();
	}
	
	else {
		kprintf("%vfbrUnknown action \"%s\"\n", action);

		return -1;
	}

	return 0;
}

int sfs_cmd(const byte** argv, size_t argc) {
	if (argc <= 1) {
		kprintf("%vfbrToo few arguments, needed one argument: action\n");
		
		return -1;
	}

	const char* action = argv[1];

	if (strcmp(action, "help") == 0) {
		kprintf("%vfbyUsage:\n");

		kprintf("\tsfs [action (format)]\n");
		
		return 0;
	}
	
	else if (strcmp(action, "format") == 0) {
		return sfs_format(ATA_MASTER);
	}
		
	return 0;
}
