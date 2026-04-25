#ifndef _EMULATOR_VGA_H
#define _EMULATOR_VGA_H

#include "types.h"

#include "emulator_fwd.h"

#include "memory/emulator_ram.h"

#define VGA_CHAR_WIDTH 8

#define VGA_CHAR_HEIGHT 16

typedef struct vga_text_screen_t {
	uint16* vidmem; uint64 vidmem_ram_addr;
	
	_size_t width, height, bpp;
	
	_size_t cursor_pos;

	byte crt_reg_a;

	byte mode_reg;

	bool gui;

	#ifdef EMULATOR_SDL_USING
	byte* font;
	_size_t font_width;
	_size_t font_height;

	uint32* emulator_screen;
	_size_t emulator_screen_width;
	_size_t emulator_screen_height;
	#endif
} vga_text_screen_t;

vga_text_screen_t* init_vga_text_screen(uint32* screen, _size_t screen_width, _size_t screen_height, bool gui, ram_t* ram, _ssize_t columns, _ssize_t rows);

void clear_vga_text_screen(vga_text_screen_t* screen);

void draw_vga_text_screen(vga_text_screen_t* screen);

void free_vga_text_screen(vga_text_screen_t* screen);

void release_all_vga_text_screen(vga_text_screen_t* screen);

void reset_vga_text_screen(vga_text_screen_t* screen);

void vga_text_screen_apply_attribute(byte attribute);

#endif
