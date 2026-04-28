#include "drivers/video/vga.h"

#include "drivers/memory/memory.h"

void crt_set_cursor_pos(size_t x, size_t y) {
	size_t cursor_pos = (y * COLUMNS) + x;

	out8(0x3D4, 0xE);

	out8(0x3D5, (cursor_pos >> 8) & 0xFF);

	out8(0x3D4, 0xF);

	out8(0x3D5, cursor_pos & 0xFF);
}

void crt_enable_cursor() {
	out8(0x3D4, 0x0A);
	
	out8(0x3D4, 0x0A);
}

void crt_disable_cursor() {

}

byte read_mode_reg() {
	in8(0x3DA);

	out8(0x3C0, 0x10 | 0x20);

	return in8(0x3C1);
}

void enable_blink() {
	byte mode = read_mode_reg() | 0x08;

	in8(0x3DA);

	out8(0x3C0, 0x10 | 0x20);

	out8(0x3C0, mode | 0x20);
}

void disable_blink() {
	byte mode = read_mode_reg() & (~0x08);

	in8(0x3DA);

	out8(0x3C0, 0x10 | 0x20);

	out8(0x3C0, mode | 0x20);
}
