#define __EMULATOR__

#include "kernel.h"

#include "types.h"

#include "std.h"

#include "drivers/memory/memory.h"

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
	.mode_type = 	0,
	.width = 		800,
	.height = 		600,
	.depth = 		32,
};

PUBLIC void __emulator_init_kernel(__init_kernel_args_t _kernel_args) {
	kernel_args = _kernel_args;
}

PUBLIC multiboot_section_t* __emulator_read_multiboot_secton(void) {
	return &multiboot_section;
}
#endif

byte* ram = nullptr;

void report(const c_str msg) {
	#ifdef __EMULATOR__
	if (kernel_args.__emulator_kernel_report)
		kernel_args.__emulator_kernel_report(msg);
	#endif
}

void ata_int_handler(struct registers_t* regs) {
	// kprintf("ATA ready\n");
}

static byte history[16][64] = { 0 };

static ssize_t command_index = 0;

multiboot_info_t* multiboot = nullptr;

const byte font[96][12] = {
	#include "ascii.fnt"
};

PUBLIC void kmain(uint32 magic, multiboot_info_t* _multiboot) {
	if (magic != 0x2BADB002 || !_multiboot) return;

	uint32* fb = (uint32*)(_multiboot->fb_addr);

	uint32 width = _multiboot->fb_width;
	uint32 height = _multiboot->fb_height;
	uint32 pitch = (_multiboot->fb_pitch) / sizeof(uint32);

	// TODO: ??????????? ????????? ???????????? ? std.c, ????, ??? ????? ??????? ????????? ??????????? (??? VGA) ? ???????? ??

	for (size_t i = 0; i < width; i++) {
		size_t c_i = i % 8;

		for (size_t j = 0; j < height; j++) {
			size_t c_j = j % 12;

			if (font[(i / 8) % 96][c_j] & (1ULL << (7 - c_i))) {
				fb[i + (j * pitch)] = 0x00FF00;
			}
		}
	}
	
	for (;;) halt();

	return;

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

	// sfs_format(ATA_MASTER);

	sfs_create_file(ATA_MASTER, "grub.cfg",
"set timeout=0\n\r"
"set default=0\n\r"

"menuentry \"Emulator OS (multiboot1)\" {\n\r"
"    multiboot /boot/kernel.bin\n\r"
"    boot\n\r"
"}\n\r", 116);

	sfs_create_file(ATA_MASTER, "emulator.log",
"[0.114973][INFO] OS Emulator, IBM-PC compatible (GPL V3.0) 0.3.1 (May  3 2026, 03:01:03) for Linux using Clang 18.1 x86-64 by RDev.\n"
"[0.114973][INFO] Project github: https://github.com/RandomComp/Emulator_OS\n"
"[0.114973][INFO] Running on 03 May 2026\n"
"[0.114973][INFO] Kernel \"./os.so\" loading...\n"
"[0.114973][INFO] Kernel \"./os.so\" loaded!\n"
"[0.114973][INFO] Emulator initialization...\n"
"[0.114973][VERB] SDL window and renderer initialization...\n"
"[0.344918][VERB] SDL window and renderer initialized!\n"
"[0.344918][VERB] SDL texture initialization (for drawing)...\n"
"[0.344918][VERB] SDL texture framebuffer initialization...\n"
"[0.344918][VERB] SDL texture framebuffer initialized!\n"
"[0.344918][VERB] SDL texture initialized!\n"
"[0.344918][VERB] SDL initialized!\n"
"[0.344918][VERB] CPU initialization...\n"
"[0.344918][VERB] PIC initialization...\n"
"[0.344918][VERB] PIC initialized!\n"
"[0.344918][VERB] Setup CPU signals... (SIGHUP, SIGINT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE, SIGSEGV, SIGTERM, SIGSTKFLT, SIGTSTP, SIGXCPU, SIGXFSZ, SIGWINCH)\n"
"[0.344918][VERB] CPU initialized\n"
"[0.344918][VERB] RAM initialization (800000 bytes)...\n"
"[0.354499][VERB] RAM initialized!\n"
"[0.354499][VERB] PIT initializing...\n"
"[0.354499][VERB] PIT initialized!\n"
"[0.354499][VERB] VGA text screen initialization...\n"
"[0.354499][INFO] Loading cp437 font \"cp437-8x16.png\"...\n"
"[0.354499][VERB] VGA text screen mode register (no video mode): 0b00101000\n"
"[0.354499][VERB] Clearing VGA text screen...\n"
"[0.354499][VERB] Setting up VGA screen update timer (33 hz) and text blinking timer (2 hz)...\n"
"[0.354499][VERB] Setting up ports (0x3DA, 0x3C0, 0x3C1) for VGA...\n"
"[0.354499][VERB] VGA text screen initialized\n"
"[0.354499][VERB] CMOS initialization...\n"
"[0.354499][VERB] Setting up timer (1 hz) for CMOS updating...\n"
"[0.354499][VERB] Setting up ports (0x70, 0x71) for CMOS...\n"
"[0.354499][VERB] CMOS initialized\n"
"[0.354499][VERB] PS/2 Keyboard initialization...\n"
"[0.354499][VERB] Setting up ports (0x60, 0x64) for PS/2 Keyboard...\n"
"[0.354499][VERB] PS/2 Keyboard initialized\n"
"[0.354499][VERB] ATA PIO initializing...\n"
"[0.354499][VERB] ATA PIO setting up ports (ATA_STATUS, ATA_COMMAND, ATA_DRIVE_SEL, ATA_SECTOR_COUNT, ATA_LBA_LOW, ATA_LBA_MID, ATA_LBA_HIGH, ATA_DATA)\n"
"[0.354499][VERB] Creating ATA HDD with size 32768 kb...\n"
"[0.354499][VERB] Created ATA HDD with size 32768 kb\n"
"[0.354499][VERB] HDD and ATA PIO initialized\n"
"[0.354499][VERB] Power control initialization...\n"
"[0.354499][VERB] Setting up ports (0xB004, 0x604, 0x4004, 0x600) for power off on emulators...\n"
"[0.354499][VERB] Power control initialization done!\n"
"[0.354499][INFO] Emulator initialized\n"
"[0.364080][INFO] Kernel initializing...\n"
"[0.364080][INFO] Kernel initialized!\n"
"[0.364080][INFO] Initialization duration: 247157 us\n"
"[0.364080][INFO] Emulator running...\n"
"[0.364080][VERB] Multiboot initialization...\n"
"[0.364080][VERB] Multiboot initialized\n"
"[0.364080][INFO] [KERNEL CALL] IDT Flushing...\n"
"[0.364080][VERB] [KERNEL CALL] Entries cnt: 255\n"
"[0.364080][INFO] [KERNEL CALL] IDT Flushed!\n"
"[16.575211][INFO] Exiting emulator because pressed F10...\n"
"[16.594372][VERB] CPU deinitialization...\n"
"[16.594372][VERB] PIC deinitialization...\n"
"[16.594372][VERB] PIC deinitialized!\n"
"[16.594372][VERB] CPU deinitialized\n"
"[16.594372][VERB] PS/2 Keyboard deinitialization...\n"
"[16.594372][VERB] PS/2 Keyboard deinitialized\n"
"[16.594372][VERB] HDD ATA PIO deinitializing...\n"
"[16.594372][VERB] HDD ATA PIO deinitialized\n"
"[16.594372][VERB] CMOS deinitialization...\n"
"[16.594372][VERB] CMOS deinitialized\n"
"[16.594372][VERB] Releasing timer updating vga screen (33 hz) and text blinking timer (2 hz)...\n"
"[16.594372][VERB] Releasing VGA ports (0x3DA, 0x3C0, 0x3C1)...\n"
"[16.594372][VERB] VGA text screen deinitialization...\n"
"[16.594372][VERB] VGA font deinitialization...\n"
"[16.594372][VERB] VGA font deinitialized!\n"
"[16.594372][VERB] VGA text screen deinitialized\n"
"[16.594372][VERB] RAM deinitialization...\n"
"[16.594372][VERB] RAM deinitialized\n"
"[16.594372][VERB] Power control deinitialization...\n"
"[16.594372][VERB] Releasing ports (0xB004, 0x604, 0x4004, 0x600) for power off on emulators...\n"
"[16.594372][VERB] Power control deinitialization done!\n"
"[16.594372][VERB] SDL texture framebuffer deinitialization...\n"
"[16.594372][VERB] SDL texture framebuffer deinitialized!\n"
"[16.594372][VERB] SDL texture deinitialization...\n"
"[16.594372][VERB] SDL texture deinitialized!\n"
"[16.594372][VERB] SDL renderer deinitialization...\n"
"[16.594372][VERB] SDL renderer deinitialized!\n"
"[16.594372][VERB] SDL window deinitialization...\n"
"[16.603952][VERB] SDL window deinitialized!\n", 4484);

	sfs_create_file(ATA_MASTER, "cmos.c",
"#include \"drivers/time/cmos.h\"\n"
"\n"
"#include \"types.h\"\n"
"\n"
"#include \"drivers/memory/memory.h\"\n"
"\n"
"#include \"bcd.h\"\n"
"\n"
"#include \"std.h\"\n"
"\n"
"void cmos_write_reg(byte reg, byte val) {\n"
"	out8(0x70, reg);\n"
"\n"
"	out8(0x71, val);\n"
"}\n"
"\n"
"byte cmos_read_reg(byte reg) {\n"
"	out8(0x70, reg);\n"
"\n"
"	return in8(0x71);\n"
"}\n"
"\n"
"bool cmos_update_in_progress() {\n"
"	byte reg_a = cmos_read_reg(0xA);\n"
"\n"
"	return (reg_a & CMOS_REGISTER_A_UPDATE_IN_PROGRESS) != 0;\n"
"}\n"
"\n"
"void show_rtc_time() {\n"
"	byte reg_b = cmos_read_reg(0x0B);\n"
"\n"
"	byte second = cmos_read_reg(0x00);\n"
"\n"
"	byte minute = cmos_read_reg(0x02);\n"
"\n"
"	byte hour = cmos_read_reg(0x04);\n"
"\n"
"	bool pm = false;\n"
"\n"
"	if ((reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {\n"
"		pm = hour & 0x80;\n"
"\n"
"		hour &= 0x7F;\n"
"	}\n"
"\n"
"	if ((reg_b & CMOS_REGISTER_B_IS_BINARY_MODE) == 0) {\n"
"		second = from_bcd(second);\n"
"\n"
"		minute = from_bcd(minute);\n"
"\n"
"		hour = from_bcd(hour);\n"
"	}\n"
"\n"
"	kprintf(\"%.2i:%.2i:%.2i\", hour, minute, second);\n"
"\n"
"	if ((reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {\n"
"		if (pm) kprint(\" PM\");\n"
"\n"
"		else kprint(\" AM\");\n"
"	}\n"
"}\n"
"\n"
"void show_rtc_date() {\n"
"	byte reg_b = cmos_read_reg(0x0B);\n"
"\n"
"	byte day = cmos_read_reg(CMOS_RTC_DAY_OF_MONTH);\n"
"\n"
"	byte month = cmos_read_reg(CMOS_RTC_MONTHS);\n"
"\n"
"	byte year = cmos_read_reg(CMOS_RTC_YEARS);\n"
"\n"
"	if ((reg_b & CMOS_REGISTER_B_IS_BINARY_MODE) == 0) {\n"
"		day = from_bcd(day);\n"
"\n"
"		month = from_bcd(month);\n"
"\n"
"		year = from_bcd(year);\n"
"	}\n"
"\n"
"	kprintf(\"%.2i.%.2i.%.4i\", (int)day, (int)month, (int)(year + 2000));\n"
"}\n"
"\n"
"byte read_rtc_seconds() {\n"
"	byte reg_b = cmos_read_reg(0x0B);\n"
"\n"
"	byte second = cmos_read_reg(0x00);\n"
"\n"
"	if ((reg_b & CMOS_REGISTER_B_IS_BINARY_MODE) == 0) {\n"
"		second = from_bcd(second);\n"
"	}\n"
"\n"
"	return second;\n"
"}\n", 1603);

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

