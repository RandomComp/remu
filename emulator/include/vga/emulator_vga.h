#ifndef _EMULATOR_VGA_H
#define _EMULATOR_VGA_H

#include "types.h"

#include "memory/emulator_ram.h"

typedef struct vga_text_screen_t {
	uint16* vidmem; uint64 vidmem_ram_addr;
	
	_size_t width, height, bpp;
	
	_size_t cursor_pos;

	byte crt_reg_a;

	byte mode_reg;
} vga_text_screen_t;

vga_text_screen_t* init_vga_text_screen(ram_t* ram, _ssize_t columns, _ssize_t rows);

void clear_vga_text_screen(vga_text_screen_t* screen);

void draw_vga_text_screen(vga_text_screen_t* screen);

void free_vga_text_screen(vga_text_screen_t* screen);

void release_all_vga_text_screen(vga_text_screen_t* screen);

void reset_vga_text_screen(vga_text_screen_t* screen);

void vga_text_screen_apply_attribute(byte attribute);

#endif