#include "kernel.h"

#include "types.h"

#include "std.h"

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

#include "emush/emush.h"

#ifdef __EMULATOR__
__init_kernel_args_t kernel_args = { 0 };

static multiboot_section_t multiboot_section = {
	.magic = 		0x1BADB002,
	.flags = 		0b00000111,
	.checksum = 	-(0x1BADB002 + 0b00000111),
	.mode_type = 	1,
	.width = 		80,
	.height = 		25,
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

void ata_int_handler(struct registers_t* regs) {
	// kprintf("ATA ready\n");
}

static byte history[16][64] = { 0 };

static ssize_t command_index = 0;

multiboot_info_t* multiboot = nullptr;

// const byte font[96][12] = {
// 	#include "ascii.fnt"
// };

PUBLIC void kmain(uint32 magic, multiboot_info_t* _multiboot) {
	if (magic != 0x2BADB002 || !_multiboot) return;

	// uint32* fb = (uint32*)(_multiboot->fb_addr);

	// uint32 width = _multiboot->fb_width;
	// uint32 height = _multiboot->fb_height;
	// uint32 pitch = (_multiboot->fb_pitch) / sizeof(uint32);

	// // TODO: ??????????? ????????? ???????????? ? std.c, ????, ??? ????? ??????? ????????? ??????????? (??? VGA) ? ???????? ??

	// for (size_t i = 0; i < width; i++) {
	// 	size_t c_i = i % 8;

	// 	for (size_t j = 0; j < height; j++) {
	// 		size_t c_j = j % 12;

	// 		if (font[(i / 8) % 96][c_j] & (1ULL << (7 - c_i))) {
	// 			fb[i + (j * pitch)] = 0x00FF00;
	// 		}
	// 	}
	// }
	
	// for (;;) halt();

	// return;

	multiboot = _multiboot;

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

	// byte name[64] = { 0 };

	// byte second_name[64] = { 0 };

	// int num = 10;

	// sscanf("12345 hello3289239823092389 test ", "%i %s %s", &num, name, second_name);

	// kprintf("\"%9s\"\n", "12345");

	// sfs_format(ATA_MASTER);

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

