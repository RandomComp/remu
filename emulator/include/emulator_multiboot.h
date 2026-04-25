#ifndef _EMULATOR_MULTIBOOT_H
#define _EMULATOR_MULTIBOOT_H

#define FB_TYPE_INDEXED 0

#define FB_TYPE_RGB 1

#define FB_TYPE_EGA_TEXT 2
	
#define MULTIBOOT_MEMORY_AVAILABLE 1

#define MULTIBOOT_MEMORY_RESERVED 2

#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3

#define MULTIBOOT_MEMORY_NVS 4

#define MULTIBOOT_MEMORY_BADRAM 5

#include "types.h"

// typedef struct multiboot_aout_symbol_table_t {
// 	uint32 tabsize;

// 	uint32 strsize;

// 	uint32 addr;

// 	uint32 reserved;
// } multiboot_aout_symbol_table_t;

// typedef struct multiboot_elf_section_header_table_t {
// 	uint32 num;

// 	uint32 size;

// 	uint32 addr;

// 	uint32 shndx;
// } multiboot_elf_section_header_table_t;

// typedef struct multiboot_info_t {
// 	/*
// 	Битовая маска активных полей сруктуры multiboot

// 	0 бит -- mem_lower и mem_upper действительны
// 	1 бит -- boot_device действителен
// 	2 бит -- cmdline действителен
// 	3 бит -- mods_count и mods_addr действительны
// 	4 бит -- таблица символов a.out действительна
// 	5 бит -- таблица символов elf действительна
// 	6 бит -- карта памяти mmap_length и mmap_addr действительны
// 	7 бит -- drives_length и drives_addr действительны
// 	8 бит -- config_table действительна
// 	9 бит -- boot_loader_name действителен
// 	10 бит -- APM (Advanced Power Management) действителен
// 	11 бит -- VBE (VESA BIOS Extensions) vbe_* действительны
// 	12 бит -- информация о framebuffer, fb_* действительны
// 	*/
// 	uint32 flags;

// 	uint32 mem_lower;

// 	uint32 mem_upper;

// 	/*
// 	Если 1 бит в flags не установлен, то это поле считается недействительным
	
// 	3 байт -- номер BIOS-накопителя (0x00 - первый дисковод Floppy, 0x80 (первый жесткий диск HDD/SSD), 0x81 второй жесткий диск и т.д до 0x8F)
// 	2 байт -- номер основного (физического, 0-3) раздела, если не используется 0xFF
// 	1 байт -- номер логического (виртуального, 00-FF) раздела, если не используется 0xFF
// 	0 байт -- номер следующего вложенного раздела, обычно 0xFF (не используется)
// 	*/
// 	uint32 boot_device;

// 	uint32 cmdline;

// 	uint32 mods_count;
	
// 	uint32 mods_addr;

// 	union {
// 		struct multiboot_aout_symbol_table_t aout_sym;

// 		struct multiboot_elf_section_header_table_t elf_sec;
// 	};

// 	uint32 mmap_length;
	
// 	uint32 mmap_addr;

// 	uint32 drives_length;
	
// 	uint32 drives_addr;

// 	uint32 config_table;

// 	uint32 boot_loader_name;

// 	uint32 apm_table;

// 	uint32 vbe_control_info;

// 	uint32 vbe_mode_info;

// 	uint16 vbe_mode;

// 	uint16 vbe_interface_seg;

// 	uint16 vbe_interface_off;

// 	uint16 vbe_interface_len;

// 	uint64 fb_addr;

// 	uint32 fb_pitch;

// 	uint32 fb_width;

// 	uint32 fb_height;

// 	uint8 fb_bpp;

// 	uint8 fb_type;

// 	union {
// 		struct {
// 			uint32 fb_palette_addr;

// 			uint16 fb_palette_num_colors;
// 		};

// 		struct {
// 			uint8 fb_red_field_position;

// 			uint8 fb_red_mask_size;

// 			uint8 fb_green_field_position;

// 			uint8 fb_green_mask_size;

// 			uint8 fb_blue_field_position;
			
// 			uint8 fb_blue_mask_size;
// 		};
// 	};
// } multiboot_info_t;

// typedef struct multiboot_mmap_entry_t {
// 	uint32 size;

// 	uint32 addr_low;

// 	uint32 addr_high;

// 	uint32 len_low;

// 	uint32 len_high;

// 	uint32 type;
// } multiboot_mmap_entry_t __attribute__((packed));

#endif
