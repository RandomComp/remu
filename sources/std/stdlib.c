#include "std/stdlib.h"

#include "types.h"

#include "colors.h"

#include "builtins/string.h"

#include "drivers/video/vga.h"

#include "drivers/hid/kbdps2.h"

#include "builtins/builtins.h"

#include "math/math.h"

#include "kernel.h"

static byte cur_style = 0;

terminal_out_t stdout = { 0 };
terminal_in_t stdin = { 0 };

void init_std(terminal_out_t _stdout, terminal_in_t _stdin) {
	set_cursor_pos(0, 0, false);
	set_cursor_pos(0, 0, true);

	stdout = _stdout;
	stdin = _stdin;

	cur_style = 0x0F;
}

void set_style(byte style) {
	cur_style = style;
}

byte get_style(void) {
	return cur_style;
}

size_t get_columns(void) {
	return stdout.columns;
}

size_t get_rows(void) {
	return stdout.rows;
}

void clear_screen(byte style) {
	size_t columns = get_columns();

	size_t rows = get_rows();

	set_cursor_pos(0, 0, false);

	for (size_t i = 0; i < columns * rows; i++) {
		set_style(style);

		putch(' ');
	}

	set_cursor_pos(0, 0, false);
	// set_cursor_pos(0, 0, true);
}

void clear_line() {
	ssize_t old_x = 0, old_y = 0;

	get_cursor_pos(&old_x, &old_y);

	for (size_t i = 0; i < (get_columns() - old_x); i++) {
		putch(' ');
	}

	set_cursor_pos(old_x, old_y, false);
	// set_cursor_pos(old_x, old_y, true);
}

uintmax_t get_num_digits(uintmax_t num, uintmax_t base, bool signable) {
	uintmax_t result = 0;

	if (num < 10) return 0;

	if (signable) {
		uintmax_t mask = 1ULL << ((sizeof(num) * 8ULL) - 1ULL);

		bool sign = num & mask;
					
		if (sign) {
			num = (~num) + 1; // Converting to positive number to negative, and vice versa
		}
	}

	while (num >= base) {
		result += 1;

		num /= base;
	}

	return result;
}

void set_cursor_pos(ssize_t _x, ssize_t _y, bool view) {
	if (_x == -1 && _y == -1) {
		if (stdout.disable_view_cursor)
			stdout.disable_view_cursor();
		
		return;
	}

	if (view) {
		if (stdout.enable_view_cursor)
			stdout.enable_view_cursor();
	}

	ssize_t columns = (ssize_t)get_columns();
	ssize_t rows 	= (ssize_t)get_rows();

	if (_x == -1)
		_x = columns;
	
	if (_y == -1)
		_y = rows;

	ssize_t pos = _x + (_y * columns);

	ssize_t x = columns > 0 ? pos % columns : 0;
	ssize_t y = columns > 0 ? pos / columns : 0;

	if (stdout.set_cur_pos) {
		stdout.set_cur_pos(x, y, view);
	}
}

void get_cursor_pos(ssize_t* x, ssize_t* y) {
	if (stdout.get_cur_pos) {
		stdout.get_cur_pos(x, y);
	}
}
