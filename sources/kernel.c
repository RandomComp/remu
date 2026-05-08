#include "kernel.h"

#include "types.h"

#include "std.h"

#include "terminal.h"

#include "drivers/memory/memory.h"
#include "drivers/io.h"

#include "colors.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "multiboot.h"

#include "gdt.h"

#include "idt.h"

#include "drivers/hid/kbdps2.h"

#include "drivers/ata/ata.h"

#include "drivers/sfs/sfs.h"

#include "drivers/video/vga.h"

#include "emush/emush.h"

#ifdef __EMULATOR__
__init_kernel_args_t kernel_args = { 0 };

static multiboot_section_t multiboot_section = {
	.magic = 		0x1BADB002,
	.flags = 		0b00000111,
	.checksum = 	-(0x1BADB002 + 0b00000111),
	.mode_type = 	1,
	.width = 		VGA_COLUMNS,
	.height = 		VGA_ROWS,
	.depth = 		16,
};

PUBLIC void __emulator_init_kernel(__init_kernel_args_t _kernel_args) {
	kernel_args = _kernel_args;
}

PUBLIC multiboot_section_t* __emulator_read_multiboot_secton(void) {
	return &multiboot_section;
}
#endif

byte* ram = nullptr;

static byte history[16][64] = { 0 };

static ssize_t command_index = 0;

multiboot_info_t* multiboot = nullptr;

const byte font[96][12] = {
	#include "ascii.fnt"
};

void report(const c_str msg) {
	for (size_t i = 0; msg[i]; i++) {
		out8(0x80, msg[i]);
	}

	out8(0x80, '\n');
}

PUBLIC void kmain(uint32 magic, multiboot_info_t* _multiboot) {
	if (magic != 0x2BADB002 || !_multiboot) return;

	multiboot = _multiboot;

	ram = (byte*)get_ram();

	init_vga();

	terminal_out_t stdout = init_vga_stdout();

	kbdps2_init();

	terminal_in_t stdin = init_kbdps2_stdin();

	init_std(stdout, stdin);

	byte style = COLOR_BRIGHT_WHITE | (COLOR_BLACK << 4);

	clear_screen(style);

	set_style(style);

	kprintf("%vfbyGDT initialization...");

	gdt_init();

	kprintf(" %vfbgdone\n");

	kprintf("%vfbyIDT initialization...");

	idt_init();

	kprintf(" %vfbgdone\n");

	uintmax_t result = 10;

	size_t readed_len = 0;

	int err = parse_num("RDEV", 28, &result, &readed_len); // 0123456789ABCDEFGHIJKLMNOPQR

	kprintf("parse_num err: %i\n", err);
	kprintf("parse_num readed len: %zu\n", readed_len);
	kprintf("parse_num result: %n\n", result, 28);

	time_cmd(nullptr, 0);

	while (true) {
		kprintf(get_var("PS1"));

		emush_get_history(history);

		command_index = emush_get_command_index();

		size_t cnt = 0;
		
		byte* buf = getstr_hist(true, &cnt, history, command_index, 16);

		emush_exec(buf, cnt);
	}

	for (;;) halt();
}
