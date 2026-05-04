#include "types.h"

#include "kernel.h"

#include "multiboot.h"

#include "drivers/ata/ata.h"

#include "builtins/string.h"

#include "drivers/video/vga.h"

#include "drivers/memory/memory.h"

#include "std.h"

#include "emush/emush.h"

extern byte* ram;

extern multiboot_info_t* multiboot;

const c_str logo[] =
{
	"%vfbrллллллл%vd %vfbgллл    ллл%vd %vfbyлл    лл%vd        лллллл  ллллллл\n", 
	"%vfbrлл     %vd %vfbgлллл  лллл%vd %vfbyлл    лл%vd       лл    лл лл     \n",
	"%vfbrллллл  %vd %vfbgлл лллл лл%vd %vfbyлл    лл%vd ллллл лл    лл ллллллл\n",
	"%vfbrлл     %vd %vfbgлл  лл  лл%vd %vfbyлл    лл%vd       лл    лл      лл\n", 
	"%vfbrллллллл%vd %vfbgлл      лл%vd %vfby лллллл %vd        лллллл  ллллллл\n"
};

const c_str view_logo[] =
{
	"ллллллл ллл    ллл лл    лл        лллллл  ллллллл", 
	"лл      лллл  лллл лл    лл       лл    лл лл     ",
	"ллллл   лл лллл лл лл    лл ллллл лл    лл ллллллл",
	"лл      лл  лл  лл лл    лл       лл    лл      лл", 
	"ллллллл лл      лл  лллллл         лллллл  ллллллл"
};

const c_str version_logo[] =
{
	"лл    лл  лллллл     лллллл  лллллл \n", 
	"лл    лл лл  лллл         лл      лл\n", 
	"лл    лл лл лл лл     ллллл   ллллл \n", 
	" лл  лл  лллл  лл         лл лл     \n", 
	"  лллл    лллллл  лл лллллл  ллллллл\n"
};

int info_cmd(const byte **argv, size_t argc) {
	bool is_bootloader_name_available = (multiboot->flags & 0x200) != 0;

	bool is_fb_available = (multiboot->flags & 0x1000) != 0;

	// kprintf(logo); kprintf("\n\n");

	// kprintf(version_logo); kprintf("\n\n");

	byte* info_names[] = {
		"Boot loader name",
		"RAM size",
		"Screen size",
		"Colors",
		"Video memory address",
		"OS Loaded from",
		"Sectors count",
		"Model name",
		"Serial number"
	};

	byte* info[9][128] = {
		{ 0 }
	};
	
	sprintf(info[0], "%s", is_bootloader_name_available ? (ram + (size_t)multiboot->boot_loader_name) : "N/A");

	size_t ram_size = get_ram_size(multiboot);

	sprintf(info[1], "%zu MB", ram_size / 0x100000);

	disable_blink();

	if (is_fb_available) {
		sprintf(info[2], "%u%wx%u", multiboot->fb_width, multiboot->fb_height);
	}

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
		
		sprintf(info[3], "%zu", colors);
	}
	
	if (is_fb_available)
		sprintf(info[4], "%lx", multiboot->fb_addr);

	bool is_boot_device_available = (multiboot->flags & 0x02) != 0;

	if (is_boot_device_available) {
		byte bios_num = (multiboot->boot_device >> 24) & 0xFF;

		bool is_floppy = bios_num == 0x00;

		bool is_hard = bios_num & 0x80;
		byte hard_num = bios_num & 0x0F;

		byte main_part = (multiboot->boot_device >> 16) & 0xFF;

		byte logic_part = (multiboot->boot_device >> 8) & 0xFF;

		size_t buf_index = 0;

		if (is_floppy)
			buf_index += sprintf(info[5] + buf_index, "floppy ");
		else if (is_hard) {
			buf_index += sprintf(info[5] + buf_index, "hard disk #%u ", hard_num);
		}

		if (main_part != 0xFF) {
			if (logic_part == 0xFF) {
				buf_index += sprintf(info[5] + buf_index, "part %u ", main_part);
			}

			else {
				buf_index += sprintf(info[5] + buf_index, "logical part %u on part %u ", logic_part, main_part);
			}
		}
	}

	uint32 sectors = 0;

	ata_read_info(ATA_MASTER, &sectors, info[7], info[8]);

	size_t mb = sectors / (1024 * 2);

	sprintf(info[6], "%u (%zu mb)", sectors, mb);

	size_t max_info_name_len = 0;

	size_t info_cnt = sizeof(info_names) / sizeof(info_names[0]);

	for (size_t i = 0; i < info_cnt; i++) {
		size_t info_name_len = strlen(info_names[i]);

		if (info_name_len > max_info_name_len)
			max_info_name_len = info_name_len;
	}

	size_t max_info_len = 0;

	for (size_t i = 0; i < info_cnt; i++) {
		size_t info_len = strlen(info[i]);

		if (info_len > max_info_len)
			max_info_len = info_len;
	}

	max_info_len += 2;

	max_info_name_len += 2;

	#define EMULATOR_NAME_STR "Emulator OS System Info"

	size_t center = (COLUMNS / 2) - (max_info_name_len + max_info_len + 1) / 2;

	kprintf("%*sк%0mФ*sП\n", center, "", max_info_name_len + max_info_len + 1, "");

	kprintf("%*sГ%vfbq%=*s%vdГ\n", center, "", max_info_name_len + max_info_len + 1, EMULATOR_NAME_STR);

	kprintf("%*sУ%0mФ*sТ%0mФ*sД\n", center, "", max_info_name_len, "", max_info_len, "");

	kprintf("%*sГ%=*sГ%=*sГ\n", center, "", max_info_name_len, "Category", max_info_len, "Information");

	kprintf("%*sУ%0mФ*sХ%0mФ*sД\n", center, "", max_info_name_len, "", max_info_len, "");

	for (size_t i = 0; i < info_cnt; i++) {
		kprintf("%*sГ %vfby%*s%vd Г %vfbg%-*s%vd Г\n", center, "", max_info_name_len - 2, info_names[i], max_info_len - 2, info[i]);
	}

	kprintf("%*sР%0mФ*sС%0mФ*sй\n", center, "", max_info_name_len, "", max_info_len, "");

	size_t y = 0;

	get_cursor_pos(nullptr, &y);

	set_cursor_pos((COLUMNS / 2) - 16 / 2, y);
	
	for (size_t i = 0; i < 8; i++) {
		set_style(i << 4); kprint("  ");
	}

	set_style(0x0F);

	set_cursor_pos((COLUMNS / 2) - 16 / 2, y + 1);
	
	for (size_t i = 8; i < 16; i++) {
		set_style(i << 4); kprint("  ");
	}

	set_style(0x0F);

	set_cursor_pos(0, y + 3);

	#define note_message "Note: to see more information about ATA and SFS use:"

	kprintf("%*s" note_message "\n", center, "");

	kprintf("%*s%=*s\n\r", center, "", strlen(note_message) - 1, "\"ata info\" for ata information");

	kprintf("%*s%=*s\n\r", center, "", strlen(note_message) - 1, "\"sfs info\" for sfs information");

	kprint("\n");

	return 0;
}

int logo_cmd(const byte **argv, size_t argc) {
	for (size_t i = 0; i < sizeof(logo) / sizeof(logo[0]); i++) {
		kprintf("%*s", (COLUMNS / 2) - (strlen(view_logo[i]) / 2), ""); kprintf(logo[i]);
	}

	putch('\n');

	for (size_t i = 0; i < sizeof(version_logo) / sizeof(version_logo[0]); i++) {
		kprintf("%*s", (COLUMNS / 2) - (strlen(version_logo[i]) / 2), ""); kprintf(version_logo[i]);
	}

	return 0;
}

int readme_cmd(const byte **argv, size_t argc) {
	kprintf("This is my OS created from scratch "
			"during the development of an emulator with paravirtualization support\n");

	return 0;
}
