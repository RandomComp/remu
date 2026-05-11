#include "drivers/video/vga.h"

#include "drivers/io.h"

#include "drivers/memory/memory.h"

#include "builtins/string.h"

#include "terminal.h"

#include "math/math.h"

#include "std.h"

static uint16* vidmem = nullptr;

static ssize_t cur_x = 0, cur_y = 0;

void init_vga() {
	vidmem = (byte*)get_ram() + 0xB8000;
}

void set_cur_pos(ssize_t column, ssize_t row, bool view) {
	if (view) {
		crt_set_cursor_pos(column, row);
	}

	else {
		cur_x = column; cur_y = row;
	}
}

void get_cur_pos(ssize_t* column, ssize_t* row) {
	if (column) *column = cur_x;

	if (row) *row = cur_y;
}

static void enable_view_cursor(void) {
	crt_enable_cursor(15, 15);
}

static void disable_view_cursor(void) {
	crt_disable_cursor();
}

terminal_out_t init_vga_stdout(void) {
	return (terminal_out_t){
		.columns = VGA_COLUMNS,
		.rows = VGA_ROWS,
		.putch = vga_putch,
		.set_cur_pos = set_cur_pos,
		.get_cur_pos = get_cur_pos,
		.enable_view_cursor = enable_view_cursor,
		.disable_view_cursor = disable_view_cursor,
	};
}

void vga_putch(byte style, byte c) {
	if (!vidmem) return;

	ssize_t cur_pos = (cur_y * VGA_COLUMNS) + cur_x;

	if (cur_pos < 0) {
		return;
	}
	
	switch (c) {
		case '\b':
			cur_x -= 1; break;

		case '\t':
			cur_x += 4; break;
		
		case '\n':
			cur_y += 1; break;
		
		case '\r':
			cur_x = 0; break;
		
		default:
			if (cur_pos >= 0 && cur_pos < VGA_VIDMEM_SIZE) {
				vidmem[cur_pos] = ((uint16)style << 8) | c;
			}

			cur_x += 1;

			break;
	}

	while (cur_y >= VGA_ROWS) {
		for (size_t i = 0; i < VGA_ROWS - 1; i++) {
			size_t next_i = i + 1;

			memcpy(vidmem + (i * VGA_COLUMNS), vidmem + (next_i * VGA_COLUMNS), VGA_COLUMNS * 2);
		}

		memset(vidmem + (VGA_ROWS - 1) * VGA_COLUMNS, 0, VGA_COLUMNS * 2);

		cur_y--;
	}
}

void crt_set_cursor_pos(ssize_t x, ssize_t y) {
	ssize_t cursor_pos = (y * VGA_COLUMNS) + x;

	out8(0x3D4, 0xE);
	out8(0x3D5, (cursor_pos >> 8) & 0xFF);

	out8(0x3D4, 0xF);
	out8(0x3D5, cursor_pos & 0xFF);
}

void crt_enable_cursor(byte cursor_start, byte cursor_end) {
	out8(0x3D4, 0x0A);
	out8(0x3D5, (in8(0x3D5) & 0xC0) | cursor_start);

	out8(0x3D4, 0x0B);
	out8(0x3D5, (in8(0x3D5) & 0xE0) | cursor_end);
}

void crt_disable_cursor() {
	out8(0x3D4, 0x0A);
	
    uint8 temp = in8(0x3D5);
	
    out8(0x3D5, temp | 0x20); 
}

byte vga_read_mode_reg() {
	in8(0x3DA);

	out8(0x3C0, 0x10 | 0x20);

	return in8(0x3C1);
}

void vga_enable_blink() {
	byte mode = vga_read_mode_reg() | 0x08;

	in8(0x3DA);

	out8(0x3C0, 0x10 | 0x20);

	out8(0x3C0, mode | 0x20);
}

void vga_disable_blink() {
	byte mode = vga_read_mode_reg() & (~0x08);

	in8(0x3DA);

	out8(0x3C0, 0x10 | 0x20);

	out8(0x3C0, mode | 0x20);
}

bool vga_is_blink() {
	return (vga_read_mode_reg() & 0x08) != 0;
}
