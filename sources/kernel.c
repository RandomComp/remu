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

#ifdef __EMULATOR__
__init_kernel_args_t kernel_args = { 0 };

PUBLIC void __emulator_init_kernel(__init_kernel_args_t _kernel_args) {
	kernel_args = _kernel_args;
}
#endif

void sh_calc() {
	kprintf("Enter radix (10 default): ");

	char* typed = getstr(true);

	size_t radix = parse_num(typed, 10);

	radix = radix <= 1 ? 10 : radix;

	kprintf("Enter first number: ");

	typed = getstr(true);

	size_t first_num = parse_num(typed, radix);

	kprintf("Enter second number: ");

	typed = getstr(true);

	size_t second_num = parse_num(typed, radix);

	kprintf("Enter operator: ");

	typed = getstr(true);

	char op_c = typed[0];

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
	
	if (is_fb_available) kprintf("\tVideo memory address: %vfby%x", multiboot->fb_addr);

	kprint("\n");
}

static byte history[16][64] = { 0 };

static ssize_t command_index = 1;

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

	kprintf(logo); kprintf("\n\n");

	kprintf(version_logo); kprintf("\n\n");

	while (true) {
		kprintf("Emulator OS:$ ");

		ssize_t index = 0;

		byte* typed = getstr_hist(true, history, &command_index, 16);

		byte src_typed[64] = { 0 };

		memcpy(src_typed, typed, 64);

		c_str command = strtok(typed, " ");

		if (!command) {
			continue;
		}

		// c_str arg = strtok(nullptr, " ");

		// while (arg != nullptr) {
		// 	kprintf("\"%s\"\n", arg);
			
		// 	arg = strtok(nullptr, " ");
		// }

		bool known_command = false;

		if (strcmp(command, "help") == 0) {
			kprintf("\thelp -- see help\n");

			kprintf("\tclear -- clears screen\n");

			kprintf("\techo -- echoes back the arguments\n");

			kprintf("\treadme -- readme instruction\n");

			kprintf("\tinfo -- see information about system\n");

			kprintf("\ttime -- see system time\n");

			kprintf("\tcalc -- enter calculator\n");

			kprintf("\tshut -- shutdown system\n");
		}

		else if (strcmp(command, "clear") == 0) {
			clear_screen(0x0F);
		}

		else if (strcmp(command, "echo") == 0) {
			c_str arg = strtok(nullptr, " ");

			while (arg != nullptr) {
				kprintf(arg);
				
				arg = strtok(nullptr, " ");
			}

			kprint("\n");
		}

		else if (strcmp(command, "readme") == 0) {
			kprintf("This is my OS created from scratch "
					"during the development of an emulator with paravirtualization support\n");
		}

		else if (strcmp(command, "info") == 0) {
			show_info(multiboot);
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
