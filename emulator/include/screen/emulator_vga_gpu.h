#ifndef _EMULATOR_VGA_H
#define _EMULATOR_VGA_H

#include "types.h"

#include "emulator_fwd.h"

#include "memory/emulator_ram.h"

#include "screen/emulator_vga_fwd.h"

#include "pci/emulator_pci_fwd.h"

struct vga_text_device_t {
	uint16* vidmem; uint64 vidmem_ram_addr;

	// byte* plane2;
	
	_size_t width, height, bpp;
	
	_ssize_t cursor_pos;

	/*
	bits 0-4 is cursor start
	bit 5 is cursor disable
	bits 6-7 reserved
	*/
	byte crt_reg_a;

	/*
	bits 0-4 is cursor end
	bit 5-6 is cursor skew (ignored)
	bit 7 reserved
	*/
	byte crt_reg_b;

	byte mode_reg;

	pci_device_t* pci_device;
};

vga_text_device_t* init_vga_text_device(ram_t* ram, _ssize_t columns, _ssize_t rows);

void clear_vga_text_screen(vga_text_device_t* device);

void handle_mouse_move(size_t x, size_t y, int win_x, int win_y);

void handle_mouse_button(size_t x, size_t y, int win_x, int win_y, bool released);

int draw_vga_text(vga_text_device_t* device, const byte* text, byte style, _size_t column, _size_t row);

void free_vga_text_device(vga_text_device_t* device);

void release_all_vga_text_device(vga_text_device_t* device);

void reset_vga_text_device(vga_text_device_t* device);

#endif
