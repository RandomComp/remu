#include "drivers/video/vga.h"

#include "drivers/io.h"

#include "drivers/memory/memory.h"

#include "terminal.h"

static uint16* vidmem = nullptr;

void init_vga() {
	vidmem = get_ram() + 0xB8000;
}

terminal_out_t init_vga_stdout(void) {
	return (terminal_out_t){
		.columns = VGA_COLUMNS,
		.rows = VGA_ROWS,
		.setch = vga_setch,
		.getch = vga_getch,
		.get_style = vga_get_style
	};
}

byte vga_getch(size_t column, size_t row) {
	size_t pos = column + (row * VGA_COLUMNS);

	return vidmem[pos] & 0xFF;
}

byte vga_get_style(size_t column, size_t row) {
	size_t pos = column + (row * VGA_COLUMNS);

	return (vidmem[pos] >> 8) & 0xFF;
}

void vga_setch(size_t column, size_t row, byte style, byte c) {
	if (!vidmem) return;

	size_t pos = column + (row * VGA_COLUMNS);

	vidmem[pos] = ((uint16)style << 8) | c;
}

void crt_set_cursor_pos(size_t x, size_t y) {
	size_t cursor_pos = (y * VGA_COLUMNS) + x;

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
