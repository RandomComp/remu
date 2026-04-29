#include "kernel.h"

#include "types.h"

#include "std.h"

#include "drivers/memory/memory.h"

#include "colors.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "drivers/power.h"

#include "drivers/time/tsc.h"

#include "drivers/time/cmos.h"

#include "multiboot.h"

#include "math/math.h"

#include "drivers/video/vga.h"

#include "gdt.h"

#include "idt.h"

#include "drivers/hid/kbdps2.h"

#include "drivers/ata/ata.h"

#ifdef __EMULATOR__
__init_kernel_args_t kernel_args = { 0 };

PUBLIC void __emulator_init_kernel(__init_kernel_args_t _kernel_args) {
	kernel_args = _kernel_args;
}
#endif

void sh_calc() {
	byte buf[512] = { 0 };

	kprintf("Enter radix (10 default): ");

	getstr(true, buf, 512);

	size_t radix = parse_num(buf, 10);

	radix = radix <= 1 ? 10 : radix;

	memset(buf, 0, 512);

	kprintf("Enter first number: ");

	getstr(true, buf, 512);

	size_t first_num = parse_num(buf, radix);

	memset(buf, 0, 512);

	kprintf("Enter second number: ");

	getstr(true, buf, 512);

	size_t second_num = parse_num(buf, radix);

	memset(buf, 0, 512);

	kprintf("Enter operator: ");

	getstr(true, buf, 512);

	char op_c = buf[0];

	const char supported_ops[] = "+-*/%~&|^";

	bool supported = false;

	for (size_t i = 0; i < sizeof(supported_ops) - 1; i++) {
		if (supported_ops[i] == op_c) {
			supported = true; break;
		}
	}

	if (!supported) {
		kprintf("Operation '%c' not supported.\n", op_c);
		
		return;
	}

	int result = 0;

	switch (op_c) {
		case '+':
			result = first_num + second_num;
			break;

		case '-':
			result = first_num - second_num;
			break;

		case '*':
			result = first_num * second_num;
			break;

		case '/':
			if (second_num == 0) {
				kprintf("%vfbrError: Division by zero is an illegal operation.\n"); return;
			}
		
			result = first_num / second_num;
			break;

		case '%':
			if (second_num == 0) {
				kprintf("%vfbrError: Division by zero is an illegal operation.\n"); return;
			}
		
			result = first_num % second_num;
			break;
		case '~':
			result = (~first_num) + second_num;
			break;
		case '&':
			result = first_num & second_num;
			break;
		case '|':
			result = first_num | second_num;
			break;
		case '^':
			result = first_num ^ second_num;
			break;
		
		default:
			break;
	}

	kprintf("%i %c %i = %i\n", first_num, op_c, second_num, result);
}

static byte* ram = nullptr;

const c_str logo =
"%vfbrллллллл%vd %vfbgллл    ллл%vd %vfbyлл    лл%vd        лллллл  ллллллл\n" 
"%vfbrлл     %vd %vfbgлллл  лллл%vd %vfbyлл    лл%vd       лл    лл лл     \n" 
"%vfbrллллл  %vd %vfbgлл лллл лл%vd %vfbyлл    лл%vd ллллл лл    лл ллллллл\n" 
"%vfbrлл     %vd %vfbgлл  лл  лл%vd %vfbyлл    лл%vd       лл    лл      лл\n" 
"%vfbrллллллл%vd %vfbgлл      лл%vd %vfby лллллл %vd        лллллл  ллллллл\n";

const c_str version_logo =
"лл    лл  лллллл     лллллл      лл\n"
"лл    лл лл  лллл         лл    ллл\n"
"лл    лл лл лл лл     ллллл      лл\n"
" лл  лл  лллл  лл         лл     лл\n"
"  лллл    лллллл  лл лллллл  лл  лл\n";

void show_info(multiboot_info_t* multiboot) {
	bool is_bootloader_name_available = (multiboot->flags & 0x200) != 0;

	bool is_fb_available = (multiboot->flags & 0x1000) != 0;

	// kprintf(logo); kprintf("\n\n");

	// kprintf(version_logo); kprintf("\n\n");

	kprintf("%vfbqEmulator OS v0.3.1\n");

	kprintf("\tBoot loader name: %vfbg%s\n", is_bootloader_name_available ? (ram + (size_t)multiboot->boot_loader_name) : "N/A");

	size_t ram_size = get_ram_size(multiboot);

	kprintf("\tRAM size: %vfbg%l MB\n", ram_size / 0x100000);

	disable_blink();

	size_t y = 0;

	size_t center_x = COLUMNS - 1 - 16;

	if (is_fb_available) {
		center_x = COLUMNS - 1 - 16;

		kprintf("\tScreen size: %vfby%i%vdx%vfby%i", multiboot->fb_width, multiboot->fb_height);
	}

	get_cursor_pos(nullptr, &y);

	set_cursor_pos(center_x, y);
	
	for (size_t i = 0; i < 8; i++) {
		set_style(i << 4); kprint("  ");
	}

	set_style(0x0F);

	kprintf("\n");

	if (is_fb_available) {
		size_t colors = 0;

		if (multiboot->fb_type == FB_TYPE_EGA_TEXT) {
			colors = 1 << (multiboot->fb_bpp - 8);
		}

		else if (multiboot->fb_type == FB_TYPE_RGB) {
			colors = 1 << multiboot->fb_bpp;
		}

		else if (multiboot->fb_type == FB_TYPE_INDEXED) {
			colors = multiboot->fb_palette_num_colors;
		}
		
		kprintf("\tColors: %vfby%i", colors);
	}

	get_cursor_pos(nullptr, &y);

	set_cursor_pos(center_x, y);
	
	for (size_t i = 8; i < 16; i++) {
		set_style(i << 4); kprint("  ");
	}

	set_style(0x0F);

	kprint("\n");
	
	if (is_fb_available)
		kprintf("\tVideo memory address: %vfby%x\n", multiboot->fb_addr);

	bool is_boot_device_available = (multiboot->flags & 0x02) != 0;

	if (is_boot_device_available) {
		byte bios_num = (multiboot->boot_device >> 24) & 0xFF;

		bool is_floppy = bios_num == 0x00;

		bool is_hard = bios_num & 0x80;
		byte hard_num = bios_num & 0x0F;

		byte main_part = (multiboot->boot_device >> 16) & 0xFF;

		byte logic_part = (multiboot->boot_device >> 8) & 0xFF;

		kprintf("\tOS Loaded from ");

		if (is_floppy)
			kprintf("floppy ");
		else if (is_hard) {
			kprintf("hard disk #%i ", hard_num);
		}

		if (main_part != 0xFF) {
			if (logic_part == 0xFF) {
				kprintf("part %i ", main_part);
			}

			else {
				kprintf("logical part %i on part %i ", logic_part, main_part);
			}
		}

		kprint("(getted from multiboot)");
	}

	kprint("\n");

	uint32 sectors = 0;

	byte model_name[20] = { 0 };

	byte serial_number[10] = { 0 };

	ata_read_info(ATA_MASTER, &sectors, model_name, serial_number);

	size_t mb = sectors / (1024 * 2);

	kprintf("\tSectors count: %vfby%i%vd (%vfby%i%vd mb)\n", sectors, mb);
	
	kprintf("\tModel name: %s\n", model_name);
	
	kprintf("\tSerial number: %s\n", serial_number);

	kprint("\n");
}

void ata_cmd(const char** argv, size_t argc) {
	if (argc < 1) {
		kprintf("%vfbrToo few arguments, needing minimal one argument: action name\n");

		return;
	}

	const char* action = argv[0];

	if (strcmp(action, "help") == 0) {
		kprintf("Usage:\n");

		kprintf("\t%vfbyata [action] [sector (if write/read)] [data (if write)]\n");

		kprintf("available actions: \n");

		kprintf("\thelp -- see help\n");

		kprintf("\tinfo -- see ATA info\n");

		kprintf("\tdump -- see ATA memory dump\n");

		kprintf("\tread -- read data from ATA\n");

		kprintf("\twrite -- write data to ATA\n");

		return;
	}

	else if (strcmp(action, "info") == 0) {
		uint32 sectors = 0;

		byte model_name[20] = { 0 };

		byte serial_number[10] = { 0 };

		ata_read_info(ATA_MASTER, &sectors, model_name, serial_number);

		size_t mb = sectors / (1024 * 2);

		kprintf("\tSectors count: %vfby%i%vd (%vfby%i%vd mb)\n", sectors, mb);
		
		kprintf("\tModel name: %s\n", model_name);
		
		kprintf("\tSerial number: %s\n", serial_number);

		return;
	}

	else if (strcmp(action, "dump") == 0) {
		size_t bytes = ata_read_size(ATA_MASTER);

		size_t last_i = 0;

		for (size_t i = 0; i < bytes; i++) {
			byte data = ata_read_byte(ATA_MASTER, i);

			if (!data) continue;

			if (i != (last_i + 1)) {
				kprintf("\nData from %i: ", i);
			}

			putch(data);

			last_i = i;
		}

		kprint("\n");

		return;
	}

	if (argc < 2) {
		kprintf("%vfbrToo few arguments, needing 2 argument: action name and sector\n");

		return;
	}

	size_t sector = parse_num(argv[1], 10);

	byte buf[512] = { 0 };

	if (strcmp(action, "read") == 0) {
		char* sector_str = argv[1];

		ata_read_sector(buf, ATA_MASTER, sector, 1);

		kprintf("Data from master disk sector %i: %s\n", sector, buf);
	}
	
	else if (strcmp(action, "write") == 0) {
		if (argc < 3) {
			kprintf("%vfbrToo few arguments, needing 3 and more arguments: action name, sector and data1, data2... etc \n");

			return;
		}

		uint32 sectors = 0;

		ata_read_info(ATA_MASTER, &sectors, nullptr, nullptr);

		size_t buf_index = 0;

		for (size_t i = 0; i < argc - 2; i++) {
			buf_index += sprint(buf + buf_index, argv[i + 2]);
		}

		ata_write_sector(buf, ATA_MASTER, sector, 1);

		ata_flush();
	}
	
	else {
		kprintf("%vfbrUnknown action \"%s\"\n", action);
	}
}

void graphtest() {
	byte src_style = get_style();

	disable_blink();
	
	for (size_t i = 0; i < ROWS; i++) {
		for (size_t j = 0; j < COLUMNS; j++) {
			set_style(0x00);

			set_cursor_pos(j, i);

			float x = j; float y = i;

			x = (((float)x / (float)COLUMNS) * 2.0f) - 1.0f;
			y = (((float)y / (float)ROWS) * 2.0f) - 1.0f;

			x *= ((float)COLUMNS / (float)ROWS) / ((float)16 / (float)8);
			
			if ((x * x + y * y) < 0.5f) {
			}
			set_style((byte)((x * x + y * y) * 15.0f) << 4);

			putch(' ');
		}
	}

	set_style(src_style);

	const c_str msg = "Press 'q' to quit"; size_t msg_len = strlen(msg);

	size_t spaces = (COLUMNS / 2) - (msg_len / 2);

	for (size_t i = 0; i < spaces; i++) {
		putch(' ');
	}

	kprintf("%s\r", msg);

	byte ch = getch();

	while (ch != 'q') {
		ch = getch(); halt();
	}
}

static byte history[16][64] = { 0 };

static ssize_t command_index = 0;

void show_history() {
	for (int i = 0; i < 16; i++) {
		c_str msg = history[i];

		if (strlen(msg) > 0) {
			kprintf("\t%i. %s\n", i, msg);
		}
	}

	kprintf("\tCurrent: %i/%i\n", command_index, 16);
}

void report(const c_str msg) {
	#ifdef __EMULATOR__
	if (kernel_args.__emulator_kernel_report)
		kernel_args.__emulator_kernel_report(msg);
	#endif
}

void ata_int_handler(struct registers_t* regs) {
	// kprintf("ATA ready\n");
}

PUBLIC void kmain(uint32 magic, multiboot_info_t* multiboot) {
	if (magic != 0x2BADB002 || !multiboot) return;

	ram = (byte*)get_ram();

	init_std(ram + 0xB8000);

	byte style = COLOR_BRIGHT_WHITE | (COLOR_BLACK << 4);

	clear_screen(style);

	set_style(style);

	kprintf("%vfbyGDT initialization...");

	gdt_init();

	kprintf(" %vfbgdone\n");

	kprintf("%vfbyIDT initialization...");

	idt_init();

	kprintf(" %vfbgdone\n");

	kprintf("%vfbyKeyboard PS/2 initialization...");

	kbdps2_init();

	kprintf(" %vfbgdone\n");

	IDTIRQInstallHandler(0x0E, ata_int_handler);

	byte buf[512] = { 0 };

	while (true) {
		kprintf("emush:$ ");

		size_t cnt = getstr_hist(true, buf, 512, history, &command_index, 16);

		byte src_typed[512] = { 0 };

		memcpy(src_typed, buf, cnt);

		c_str command = strtok(buf, " ");

		char* argv[16] = { "" };

		size_t argc = 0;
		
		byte c = 0;

		if (!command) {
			continue;
		}

		c_str arg = strtok(nullptr, " ");

		while (arg != nullptr) {
			argv[argc++] = arg;

			arg = strtok(nullptr, " ");
		}

		if (strcmp(command, "help") == 0) {
			kprintf("\thelp -- see help\n");

			kprintf("\tclear -- clears screen\n");

			kprintf("\techo -- echoes back the arguments\n");

			kprintf("\tlogo -- system logo\n");

			kprintf("\treadme -- readme instruction\n");

			kprintf("\thistory -- show commands history\n");

			kprintf("\tinfo -- see information about system\n");

			kprintf("\tata -- manipulate ata drives\n");

			kprintf("\tgraphtest -- see graphical test in text mode\n");

			kprintf("\ttime -- see system time\n");

			kprintf("\tcalc -- enter calculator\n");

			kprintf("\tshut -- shutdown system\n");
		}

		else if (strcmp(command, "clear") == 0) {
			clear_screen(0x00);
			set_style(0x0F);
		}

		else if (strcmp(command, "echo") == 0) {
			for (size_t i = 0; i < argc; i++) {
				kprintf("%s\n", argv[i]);
			}

			kprint("\n");
		}

		else if (strcmp(command, "logo") == 0) {
			kprintf(logo); putch('\n');

			kprintf(version_logo);
		}

		else if (strcmp(command, "readme") == 0) {
			kprintf("This is my OS created from scratch "
					"during the development of an emulator with paravirtualization support\n");
		}

		else if (strcmp(command, "history") == 0) {
			show_history();
		}

		else if (strcmp(command, "info") == 0) {
			show_info(multiboot);
		}

		else if (strcmp(command, "ata") == 0) {
			ata_cmd(argv, argc);
		}

		else if (strcmp(command, "graphtest") == 0) {
			graphtest();
		}

		else if (strcmp(command, "time") == 0) {
			show_rtc_time(); kprint("\n");
			show_rtc_date(); kprint("\n");
		}

		else if (strcmp(command, "calc") == 0) {
			sh_calc();
		}

		else if (strcmp(command, "shut") == 0) {
			kprintf("Shutting down...\n");

			#ifndef __EMULATOR__
			poweroff();
			#endif

			return;
		}

		else if (strcmp(command, "reboot") == 0) {
			kprintf("Rebooting...\n");

			reboot();

			return;
		}

		else if (strlen(command) != 0) {
			kprintf("%vfbrUnknown command \"%s\"\n", command);
		}

		memcpy(history[command_index], src_typed, 64);

		command_index = (command_index + 1) % 16;
	}

	for (;;) halt();
}
