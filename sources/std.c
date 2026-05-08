#include "std.h"

#include "types.h"

#include "colors.h"

#include "builtins/string.h"

#include "drivers/video/vga.h"

#include "drivers/hid/kbdps2.h"

#include "builtins/builtins.h"

#include "math/math.h"

#include "kernel.h"

static terminal_out_t stdout = { 0 };

static terminal_in_t stdin = { 0 };

static ssize_t cur_x = 0, cur_y = 0;

static byte cur_style = 0;

void init_std(terminal_out_t _stdout, terminal_in_t _stdin) {
	cur_x = 0, cur_y = 0;

	stdout = _stdout;

	stdin = _stdin;

	cur_style = 0x0F;
}

byte getch(void) {
	if (stdin.getch)
		return stdin.getch();

	return 0;
}

size_t getstr(bool show, byte* buf, size_t buf_size) {
	ssize_t index = 0;

	ssize_t cur_x = 0, cur_y = 0;
		
	get_cursor_pos(&cur_x, &cur_y);

	memset(buf, 0, buf_size);

	while (index < buf_size) {
		byte c = 0;

		while (!c) {
			c = getch();

			halt();
		}

		if (c == '\r') {
			buf[index] = '\0'; index = buf_size; break;
		}

		else if (c == '\b') {
			if (index >= 1) {
				index -= 1;

				size_t last_i = strlen(buf) - 1;

				if (index == last_i) {
					buf[index] = ' ';
				}

				else for (size_t i = index; i < last_i; i++) {
					buf[i] = buf[i + 1];
				}

				buf[last_i] = '\0';
			}
		}

		else if (c == '\x1A') {
			index = MIN(index + 1, strlen(buf));
			
			crt_set_cursor_pos(cur_x + index, cur_y);
			
			continue;
		}

		else if (c == '\x1B') {
			index = MAX(0, MIN(index - 1, strlen(buf)));
			
			crt_set_cursor_pos(cur_x + index, cur_y);
			
			continue;
		}

		else {
			buf[index] = c;
			
			index += 1;
		}

		set_cursor_pos(cur_x, cur_y);
		crt_set_cursor_pos(cur_x, cur_y);
		clear_line();
		kprint(buf);
	}

	kprint("\n");

	return index;
}

#include "drivers/memory/memory.h"

static byte inputed_buf[64] = { 0 };

byte* getstr_hist(bool show, size_t* _inputed_size, byte history[][64], ssize_t command_index, size_t history_size) {
	ssize_t index = 0;

	ssize_t cur_x = 0, cur_y = 0;
		
	get_cursor_pos(&cur_x, &cur_y);

	byte* buf = history[command_index];

	memset(buf, 0, 64);

	size_t inputed_size = 0;

	bool cur_is_inputed = true;

	while (index < 64) {
		byte c = 0;

		while (!c) {
			c = getch();

			halt();
		}

		if (c == '\x18') {
			bool ok = false;

			for (ssize_t i = command_index - 1; i >= 0; i--) {
				if (strlen(history[i]) != 0) {
					command_index = i; ok = true; break;
				}
			}

			size_t buf_len = strlen(buf);

			if (ok) {
				if (cur_is_inputed) {
					inputed_size = buf_len;

					memset(inputed_buf, 0, 64);

					memcpy(inputed_buf, buf, buf_len);

					cur_is_inputed = true;
				}

				memcpy(buf, history[command_index], 64);
					
				index = strlen(buf);

				cur_is_inputed = false;
			}
		}

		else if (c == '\x19') {
			size_t history_len = 0;

			for (ssize_t i = history_size - 1; i >= 0; i--) {
				if (strlen(history[i]) != 0) {
					history_len++;
				}
			}

			bool ok = false;

			for (ssize_t i = command_index + 1; i < history_size; i++) {
				if (strlen(history[i]) != 0) {
					command_index = i; ok = true; break;
				}
			}

			size_t buf_len = strlen(buf);

			if (!ok || command_index >= history_len) {
				// kprint("\n\r"); print_hex(inputed, 0, 20);

				buf = inputed_buf;
				
				index = MIN(64, inputed_size);

				command_index = history_size;
			}

			else {
				if (cur_is_inputed) {
					inputed_size = buf_len;

					memset(inputed_buf, 0, 64);

					memcpy(inputed_buf, buf, buf_len);

					cur_is_inputed = true;
				}

				buf = history[command_index];
				
				index = strlen(buf);

				cur_is_inputed = false;
			}
		}

		else if (c == '\x1A') {
			index = MIN(index + 1, strlen(buf));
			
			crt_set_cursor_pos(cur_x + index, cur_y);
			
			continue;
		}

		else if (c == '\x1B') {
			index = MAX(0, MIN(index - 1, strlen(buf)));
			
			crt_set_cursor_pos(cur_x + index, cur_y);
			
			continue;
		}

		else if (c == '\r') {
			index = strlen(buf);

			break;
		}

		else if (c == '\b') {
			if (index >= 1) {
				index -= 1;

				size_t last_i = strlen(buf) - 1;

				if (index == last_i) {
					buf[index] = ' ';
				}

				else for (size_t i = index; i < last_i; i++) {
					buf[i] = buf[i + 1];
				}

				buf[last_i] = '\0';
			}

			cur_is_inputed = true;
		}

		else {
			if (index < strlen(buf)) {
				byte temp_buf[512] = { 0 };

				memcpy(temp_buf, buf + index, strlen(buf) - index);

				for (size_t i = index; i < strlen(buf); i++) {
					buf[i + 1] = temp_buf[i - index];
				}
			}

			buf[index] = c;
			
			index += 1;

			cur_is_inputed = true;
		}

		set_cursor_pos(cur_x, cur_y);
		crt_set_cursor_pos(cur_x + index, cur_y);
		clear_line();
		kprint(buf);
	}

	// if (inputed) {
	// 	kfree(inputed); inputed = nullptr;
	// }

	kprint("\n");

	if (_inputed_size)
		*_inputed_size = index;

	return buf;
}

size_t getstr_hist_with_auto_add_on(bool show, byte* buf, size_t buf_size, byte history[][64], ssize_t command_index, size_t history_len) {
	ssize_t index = 0;

	ssize_t cur_x = 0, cur_y = 0;
		
	get_cursor_pos(&cur_x, &cur_y);

	memset(buf, 0, buf_size);

	while (index < buf_size) {
		byte c = 0;

		while (!c) {
			c = getch();

			halt();
		}

		if (c == '\x18') {
			for (ssize_t i = (command_index) - 1; i >= 0; i--) {
				if (strlen(history[i]) != 0) {
					command_index = i; break;
				}
			}

			memcpy(buf, history[command_index], buf_size);
				
			index = strlen(buf);
		}

		else if (c == '\x19') {
			for (ssize_t i = (command_index) + 1; i < history_len; i++) {
				if (strlen(history[i]) != 0) {
					command_index = i; break;
				}
			}

			memcpy(buf, history[command_index], buf_size);
			
			index = strlen(buf);
		}

		else if (c == '\x1A') {
			index = MIN(index + 1, strlen(buf));
			
			crt_set_cursor_pos(cur_x + index, cur_y);
			
			continue;
		}

		else if (c == '\x1B') {
			index = MAX(0, MIN(index - 1, strlen(buf)));
			
			crt_set_cursor_pos(cur_x + index, cur_y);
			
			continue;
		}

		else if (c == '\r') {
			index = strlen(buf);

			break;
		}

		else if (c == '\b') {
			if (index >= 1) {
				index -= 1;

				size_t last_i = strlen(buf) - 1;

				if (index == last_i) {
					buf[index] = ' ';
				}

				else for (size_t i = index; i < last_i; i++) {
					buf[i] = buf[i + 1];
				}

				buf[last_i] = '\0';
			}
		}

		else {
			if (index < strlen(buf)) {
				byte temp_buf[512] = { 0 };

				memcpy(temp_buf, buf + index, strlen(buf) - index);

				for (size_t i = index; i < strlen(buf); i++) {
					buf[i + 1] = temp_buf[i - index];
				}
			}

			buf[index] = c;
			
			index += 1;
		}

		set_cursor_pos(cur_x, cur_y);
		crt_set_cursor_pos(cur_x + index, cur_y);
		clear_line();
		kprint(buf);
	}

	kprint("\n");

	return index;
}

byte blkgetch(void) {
	byte result = 0;

	while (!result) {
		result = getch();

		halt();
	}

	return result;
}

void set_style(byte style) {
	cur_style = style;
}

byte get_style(void) {
	return cur_style;
}

byte get_style_from(size_t column, size_t row) {
	if (stdout.get_style)
		return stdout.get_style(column, row);
	
	return 0;
}

byte getch_from(size_t column, size_t row) {
	if (stdout.getch)
		return stdout.getch(column, row);
	
	return 0;
}

size_t get_columns(void) {
	return stdout.columns;
}

size_t get_rows(void) {
	return stdout.rows;
}

void setch(size_t column, size_t row, byte style, byte c) {
	if (stdout.setch) {
		stdout.setch(column, row, style, c);
	}
}

void putch(byte c) {
	size_t columns = get_columns();

	size_t rows = get_rows();

	if (columns <= 0 || rows <= 0)
		return;

	ssize_t cur_pos = (cur_y * columns) + cur_x;
	
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
			if (cur_pos >= 0 && cur_pos < VGA_VIDMEM_SIZE)
				setch(cur_x, cur_y, cur_style, c);

			cur_x += 1;
	}

	cur_pos = CLAMP((cur_y * columns) + cur_x, 0, columns * get_rows());

	size_t column = columns > 0 ? cur_pos % columns : 0;
	size_t row = columns > 0 ? cur_pos / columns : 0;

	set_cursor_pos(column, row);

	while (cur_y >= rows) {
		for (size_t i = 0; i < rows - 1; i++) {
			size_t next_i = i + 1;

			for (size_t j = 0; j < columns; j++) {
				byte style = get_style_from(j, next_i);
				byte c = getch_from(j, next_i);

				setch(j, i, style, c);
			}
		}

		for (size_t j = 0; j < columns; j++) {
			setch(j, rows - 1, 0, 0);
		}

		cur_y--;
	}
}

void clear_screen(byte style) {
	size_t columns = get_columns();

	size_t rows = get_rows();

	set_cursor_pos(0, 0);

	for (size_t i = 0; i < columns * rows; i++) {
		set_style(style);

		putch(' ');
	}

	set_cursor_pos(0, 0);
}

void clear_line() {
	ssize_t old_x = cur_x, old_y = cur_y;

	for (size_t i = 0; i < (get_columns() - cur_x); i++) {
		putch(' ');
	}

	set_cursor_pos(old_x, old_y);
}

size_t kprint(const c_str str) {
	size_t i = 0;

	for (; str[i]; i++) {
		if (str[i] == '\n') {
			putch('\n'); putch('\r');
		}
		
		else putch(str[i]);
	}

	// crt_set_cursor_pos(cur_x, cur_y);

	return i;
}

size_t sprint(byte *c, const c_str str) {
	if (!c) return 0;

	size_t index = 0;

	for (size_t i = 0; str[i]; i++) {
		if (str[i] == '\n') {
			c[index++] = '\n';
			c[index++] = '\r';
		}
		
		else
			c[index++] = str[i];
	}

	return index;
}

size_t parse_ext_specf(const byte* format, byte def_style) {
	bool brighted = false;

	bool foreground = true;

	bool background = false;

	size_t temp_i = 0; 

	if (format[temp_i] == 'f') {
		foreground = true; background = false;

		temp_i += 1;
	}

	else if (format[temp_i] == 'b') {
		foreground = false; background = true;

		temp_i += 1;
	}

	if (format[temp_i] == 'b') {
		brighted = true;

		temp_i += 1;
	}

	if (format[temp_i] == 'd') {
		set_style(def_style);

		temp_i += 1;
	}

	else {
		const char supported_colors[] = "augqrmywi";
		
		const byte colors_available[] = { 
			COLOR_BLACK,
			COLOR_BLUE,
			COLOR_GREEN,
			COLOR_AQUA,
			COLOR_RED,
			COLOR_MAGENTA,
			COLOR_BROWN,
			COLOR_WHITE,
			0x80,
		};

		bool is_supported = false;

		size_t index = 0;

		for (index = 0; index < sizeof(supported_colors); index++) {
			if (supported_colors[index] == format[temp_i]) {
				is_supported = true;
				
				break;
			}
		}

		if (is_supported) {
			byte color = colors_available[index];

			if (color == 0x80) {
				byte cur_style = get_style();

				byte fg = cur_style & 0x0F;

				byte bg = (cur_style >> 4) & 0x0F;

				set_style(bg | (fg << 4));

				temp_i += 1;
			}

			else {
				color = color | (brighted << 3);

				if (background)
					color = (color << 4) | (get_style() & 0xF);
				else if (foreground)
					color = color | ((get_style() >> 4) & 0xF);

				set_style(color);

				temp_i += 1;
			}
		}
	}

	return temp_i;
}

ssize_t vsnprintf(byte* s, ssize_t max_len, const c_str format, va_list list) {
	ssize_t w_index = 0;

	for (size_t i = 0; format[i]; i++) {
		byte c = format[i];

		while (c == '%') {
			size_t temp_i = i + 1;

			bool handled = false;

			intmax_t alignment = 0;
			bool left_alignment = true; bool right_alignment = false; bool center_alignment = false;

			bool alignment_dots = false;

			intmax_t min_digits = 0;

			byte c_filler = ' ';

			if (format[temp_i] == '0') {
				if (format[temp_i + 1] == 'm') {
					c_filler = format[temp_i + 2];

					temp_i += 2;
				}

				else if (format[temp_i + 1] == 'v' &&
						format[temp_i + 2] == 'm') {
					c_filler = va_arg(list, byte);

					temp_i += 2;
				}

				else {
					c_filler = '0';
				}

				temp_i += 1;
				
				handled = true;
			}

			if (format[temp_i] == '-') {
				left_alignment = false; right_alignment = true; center_alignment = false;
				
				temp_i += 1;
				
				handled = true;
			}

			else if (format[temp_i] == '=') {
				left_alignment = false; right_alignment = false; center_alignment = true;

				temp_i += 1;
				
				handled = true;
			}

			if (isnum(format[temp_i])) {
				size_t len = 0;
					
				parse_num(format + temp_i, 10, &alignment, &len);

				temp_i += len;
				
				handled = true;
			}

			else if (format[temp_i] == '*') {
				alignment = va_arg(list, int);

				temp_i += 1;
				
				handled = true;
			}

			if (strncmp(format + temp_i, "...", 3) == 0) {
				alignment_dots = true;
				
				temp_i += 3;

				if (format[temp_i] == '*') {
					alignment = va_arg(list, int);

					temp_i += 1;
				}

				else if (isnum(format[temp_i])) {
					size_t len = 0;

					parse_num(format + temp_i, 10, &alignment, &len);

					temp_i += len;
				}

				handled = true;
			}
			
			if (format[temp_i] == '.') {
				if (isnum(format[temp_i + 1])) {
					size_t len = 0;
					
					parse_num(format + temp_i + 1, 10, &min_digits, &len);

					temp_i += len + 1;
				
					handled = true;
				}

				else if (format[temp_i + 1] == '*') {
					min_digits = va_arg(list, int);

					temp_i += 2;
				
					handled = true;
				}
			}

			bool printing_num = false;

			uintmax_t num = 0; bool signable = false; bool always_print_sign = false; uintmax_t radix = 10;
			
			if (format[temp_i] == '+') {
				always_print_sign = true;
				
				temp_i += 1;
				
				handled = true;
			}

			if (format[temp_i] == 'l') {
				printing_num = true;
				num = va_arg(list, unsigned long long); signable = false; radix = 10;

				temp_i += 1;
				
				handled = true;
			}

			if (format[temp_i] == 'z') {
				printing_num = true;
				num = va_arg(list, size_t); signable = false; radix = 10;

				temp_i += 1;
				
				handled = true;
			}
			
			if (format[temp_i] == 'u') {
				if (!printing_num) {
					printing_num = true;

					num = va_arg(list, unsigned int);
				}
				
				signable = false; radix = 10;

				temp_i += 1;
				
				handled = true;
			}
			
			if (format[temp_i] == 'i') {
				if (!printing_num) {
					printing_num = true;

					num = va_arg(list, int);
				}
				
				signable = true; radix = 10;

				temp_i += 1;
				
				handled = true;
			}

			if (format[temp_i] == 'w') {
				temp_i += 1;

				i = temp_i;

				c = format[i];

				continue;
			}

			if (format[temp_i] == 'n') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;

					printing_num = true;
				}

				radix = va_arg(list, unsigned int);

				if (radix < 2 || radix > 32)
					radix = 10;

				temp_i += 1;
				
				handled = true;
			}
			
			else if (format[temp_i] == 'd') {
				if (!printing_num) {
					num = va_arg(list, int); signable = true;

					printing_num = true;
				}

				radix = 10;

				temp_i += 1;
				
				handled = true;
			}
			
			else if (format[temp_i] == 'x') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;

					printing_num = true;
				}

				radix = 16;

				temp_i += 1;
				
				handled = true;
			}
			
			else if (format[temp_i] == 'b') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;

					printing_num = true;
				}

				radix = 2;

				temp_i += 1;
				
				handled = true;
			}

			else if (format[temp_i] == 'o') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;

					printing_num = true;
				}

				radix = 8;

				temp_i += 1;
				
				handled = true;
			}

			ssize_t str_len = 0;

			byte* str = nullptr;

			if (printing_num) {
				str_len = get_num_digits(num, radix, signable) + 1;

				if (!c_filler) 
					c_filler = '0';
			}

			else if (format[temp_i] == 's') {
				if (!c_filler) 
					c_filler = ' ';
				
				str = va_arg(list, byte*);
				
				if (!str) str = "(null)";

				str_len = strlen(str);
			}

			else if (format[temp_i] == 'c') {
				str_len = 1;
			}

			min_digits = min_digits == 0 ? str_len : min_digits;

			str_len = MIN(min_digits, str_len);

			ssize_t alignment_str_len = alignment_dots ? str_len : min_digits;

			if (right_alignment) {
				if (alignment > min_digits) {
					for (ssize_t j = 0; j < alignment - alignment_str_len && (max_len <= 0 || w_index < max_len); j++) {
						s[w_index++] = c_filler;
					}

					if (max_len > 0 && w_index >= max_len) {
						return w_index;
					}
				}
			}

			else if (center_alignment) {
				intmax_t cnt = align_down(alignment, 2) / 2 - align_down(alignment_str_len, 2) / 2;

				if (cnt > 0) {
					for (ssize_t j = 0; j < cnt && (max_len <= 0 || w_index < max_len); j++) {
						s[w_index++] = c_filler;
					}

					if (max_len > 0 && w_index >= max_len) {
						return w_index;
					}
				}
			}

			if (printing_num) {
				if ((min_digits - str_len) > 0) {
					for (ssize_t j = 0; j < (min_digits - str_len) && (max_len <= 0 || w_index < max_len); j++) {
						s[w_index++] = '0';
					}
				}

				w_index += snprint_num(s + w_index, max_len - w_index, num, radix, signable, always_print_sign);

				if (max_len > 0 && w_index >= max_len) {
					return w_index;
				}

				handled = true;
			}

			else if (format[temp_i] == 's') {
				if (alignment_dots && str_len > alignment) {
					alignment -= 3;
				}

				for (intmax_t j = 0; str[j] && j < (alignment_dots ? alignment : min_digits + 1) && (max_len <= 0 || w_index < max_len); j++) {
					if (str[j] == '\n') {
						s[w_index++] = '\n';
						s[w_index++] = '\r';
					}
					
					else s[w_index++] = str[j];
				}

				if ((max_len > 0 && w_index >= max_len)) {
					return w_index;
				}

				if (alignment_dots && str_len > alignment) {
					sprint(s + w_index, "...");
				}

				if (max_len > 0 && w_index >= max_len) {
					return w_index;
				}

				temp_i += 1;
				
				handled = true;
			}

			else if (format[temp_i] == 'c') {
				s[w_index++] = va_arg(list, int);

				if (max_len > 0 && w_index >= max_len) {
					return w_index;
				}

				temp_i += 1;

				handled = true;
			}

			if (left_alignment) {
				if (alignment > min_digits) {
					for (ssize_t j = 0; j < alignment - alignment_str_len; j++) {
						s[w_index++] = c_filler;
					}
				}

				if (max_len > 0 && w_index >= max_len) {
					return w_index;
				}
			}

			else if (center_alignment) {
				intmax_t cnt = align_up(alignment, 2) / 2 - align_up(alignment_str_len, 2) / 2;

				if (cnt > 0) {
					for (intmax_t j = 0; j < cnt; j++) {
						s[w_index++] = c_filler;
					}
				}

				if (max_len > 0 && w_index >= max_len) {
					return w_index;
				}
			}

			if (!handled) break;

			i = temp_i;

			c = format[i];
		}

		c = format[i];

		if (c == '\n') {
			s[w_index++] = '\n';
			s[w_index++] = '\r';

			if (max_len > 0 && w_index >= max_len) {
				return w_index;
			}
		}

		else if (c == '\0') {
			s[w_index++] = c;

			if (max_len > 0 && w_index >= max_len) {
				return w_index;
			}
			
			break;
		}
		
		else {
			s[w_index++] = c;

			if (max_len > 0 && w_index >= max_len) {
				return w_index;
			}
		}
	}

	return w_index;
}

ssize_t snprintf(byte *c, ssize_t max_len, const c_str format, ...) {
	va_list list;

	va_start(list, format);

	size_t writed = vsnprintf(c, max_len, format, list);

	va_end(list);

	return writed;
}

ssize_t vsprintf(byte* c, const c_str format, va_list list) {
	return vsnprintf(c, -1, format, list);
}

ssize_t sprintf(byte* c, const c_str format, ...) {
	va_list list;

	va_start(list, format);

	ssize_t writed = vsnprintf(c, -1, format, list);

	va_end(list);

	return writed;
}

size_t kprintf(const c_str format, ...) {
	va_list list;

	va_start(list, format);
	
	byte def_style = get_style();

	size_t w_index = 0;

	for (size_t i = 0; format[i]; i++) {
		byte* c = format + i;

		while (*c == '%') {
			size_t temp_i = i + 1;

			bool passed = false;

			intmax_t alignment = 0;
			bool left_alignment = false; bool right_alignment = true; bool center_alignment = false;

			bool alignment_dots = false;

			intmax_t min_digits = 0;

			byte c_filler = 0;

			if (format[temp_i] == '0') {
				if (format[temp_i + 1] == 'm') {
					c_filler = format[temp_i + 2];

					temp_i += 2;
				}

				else if (format[temp_i + 1] == 'v' &&
						format[temp_i + 2] == 'm') {
					c_filler = va_arg(list, byte);

					temp_i += 2;
				}

				else {
					c_filler = '0';
				}

				temp_i += 1;
			}

			if (format[temp_i] == '-') {
				left_alignment = true; right_alignment = false; center_alignment = false;
				
				temp_i += 1;
				
				passed = true;
			}

			else if (format[temp_i] == '=') {
				left_alignment = false; right_alignment = false; center_alignment = true;

				temp_i += 1;
				
				passed = true;
			}

			if (isnum(format[temp_i])) {
				size_t len = 0;
					
				parse_num(format + temp_i, 10, &alignment, &len);

				temp_i += len;
				
				passed = true;
			}

			else if (format[temp_i] == '*') {
				alignment = va_arg(list, int);

				temp_i += 1;
				
				passed = true;
			}

			if (strncmp(format + temp_i, "...", 3) == 0) {
				alignment_dots = true;
				
				temp_i += 3;

				if (format[temp_i] == '*') {
					alignment = va_arg(list, int);

					temp_i += 1;
				}

				else if (isnum(format[temp_i])) {
					size_t len = 0;

					parse_num(format + temp_i, 10, &alignment, &len);

					temp_i += len;
				}

				passed = true;
			}
			
			if (format[temp_i] == '.') {
				if (isnum(format[temp_i + 1])) {
					size_t len = 0;
					
					parse_num(format + temp_i + 1, 10, &min_digits, &len);

					temp_i += len + 1;
				
					passed = true;
				}

				else if (format[temp_i + 1] == '*') {
					min_digits = va_arg(list, int);

					temp_i += 2;
				
					passed = true;
				}
			}

			bool printing_num = false;

			uintmax_t num = 0; bool signable = false; bool always_print_sign = false; uintmax_t radix = 10;
			
			if (format[temp_i] == '+') {
				always_print_sign = true;
				
				temp_i += 1;
				
				passed = true;
			}

			if (format[temp_i] == 'l') {
				printing_num = true;

				num = va_arg(list, unsigned long long); signable = false; radix = 10;

				temp_i += 1;
				
				passed = true;
			}

			if (format[temp_i] == 'z') {
				printing_num = true;

				num = (uintmax_t)va_arg(list, size_t); signable = false; radix = 10;

				temp_i += 1;
				
				passed = true;
			}
			
			if (format[temp_i] == 'u') {
				if (!printing_num) {
					printing_num = true;

					num = va_arg(list, unsigned int);
				}
				
				signable = false; radix = 10;

				temp_i += 1;
				
				passed = true;
			}
			
			if (format[temp_i] == 'i') {
				if (!printing_num) {
					printing_num = true;

					num = va_arg(list, int);
				}
				
				signable = true; radix = 10;

				temp_i += 1;
				
				passed = true;
			}

			if (format[temp_i] == 'w') {
				temp_i += 1;

				i = temp_i;

				c = format + i;

				continue;
			}

			if (format[temp_i] == 'n') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;

					printing_num = true;
				}

				radix = va_arg(list, unsigned int);

				if (radix < 2 || radix > 32)
					radix = 10;

				temp_i += 1;
				
				passed = true;
			}
			
			else if (format[temp_i] == 'd') {
				if (!printing_num) {
					num = va_arg(list, int); signable = true;

					printing_num = true;
				}

				radix = 10;

				temp_i += 1;
				
				passed = true;
			}
			
			else if (format[temp_i] == 'x') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;

					printing_num = true;
				}

				radix = 16;

				temp_i += 1;
				
				passed = true;
			}
			
			else if (format[temp_i] == 'b') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;

					printing_num = true;
				}

				radix = 2;

				temp_i += 1;
				
				passed = true;
			}
			
			else if (format[temp_i] == 'o') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;

					printing_num = true;
				}

				radix = 8;

				temp_i += 1;
				
				passed = true;
			}

			ssize_t str_len = 0;

			byte* str = nullptr;

			if (printing_num) {
				str_len = get_num_digits(num, radix, signable) + 1;

				if (!c_filler) 
					c_filler = '0';
			}

			else if (format[temp_i] == 's') {
				if (!c_filler) 
					c_filler = ' ';
				
				str = va_arg(list, byte*);
				
				if (!str) str = "(null)";

				str_len = strlen(str);
			}
			else if (format[temp_i] == 'c') {
				str_len = 1;
			}

			min_digits = min_digits == 0 ? str_len : min_digits;

			str_len = MIN(min_digits, str_len);

			ssize_t alignment_str_len = alignment_dots ? str_len : min_digits;

			if (right_alignment) {
				if (alignment > min_digits) {
					for (ssize_t j = 0; j < alignment - alignment_str_len; j++) {
						putch(c_filler);
					}
				}
			}

			else if (center_alignment) {
				intmax_t cnt = align_down(alignment, 2) / 2 - align_down(alignment_str_len, 2) / 2;

				if (cnt > 0) {
					for (ssize_t j = 0; j < cnt; j++) {
						putch(c_filler);
					}
				}
			}

			if (printing_num) {
				if ((min_digits - str_len) > 0) {
					for (ssize_t j = 0; j < (min_digits - str_len); j++) {
						putch('0');
					}

					w_index += min_digits - str_len;
				}

				w_index += print_num(num, radix, signable, always_print_sign);
				
				passed = true;
			}

			else if (format[temp_i] == 's') {
				if (alignment_dots && str_len > alignment) {
					alignment -= 3;
				}

				for (int j = 0; str[j] && j < (alignment_dots ? alignment : min_digits + 1); j++) {
					if (str[j] == '\n') {
						putch('\n'); putch('\r');
					}
					
					else putch(str[j]);
				}

				if (alignment_dots && str_len > alignment) {
					kprint("...");
				}

				temp_i += 1;
				
				passed = true;
			}

			else if (format[temp_i] == 'c') {
				byte ch = (byte)va_arg(list, int);

				putch(ch); w_index++;

				temp_i += 1;
				
				passed = true;
			}

			else if (format[temp_i] == 'v') {
				temp_i += 1;

				temp_i += parse_ext_specf(format + temp_i, def_style);
				
				passed = true;
			}

			if (left_alignment) {
				if (alignment > min_digits) {
					for (ssize_t j = 0; j < alignment - alignment_str_len; j++) {
						putch(c_filler);
					}
				}
			}

			else if (center_alignment) {
				intmax_t cnt = align_up(alignment, 2) / 2 - align_up(alignment_str_len, 2) / 2;

				if (cnt > 0) {
					for (intmax_t j = 0; j < cnt; j++) {
						putch(c_filler);
					}
				}
			}

			if (!passed) break;

			i = temp_i;

			c = format + i;
		}

		c = format + i;

		if (*c == '\n') {
			putch('\n');
			putch('\r');
		}

		else if (*c == '\0') break;
		
		else {
			putch(*c); w_index += 1;
		}
	}
	
	set_style(def_style);

	va_end(list);

	crt_set_cursor_pos(cur_x, cur_y);

	return w_index;
}

size_t sscanf(const byte* s, const c_str format, ...) {
	va_list list;

	va_start(list, format);
	
	size_t writed_args = 0;

	size_t r_index = 0;

	for (size_t i = 0; format[i]; i++) {
		byte c = format[i];

		while (c == '%') {
			size_t temp_i = i + 1;

			bool passed = false;

			bool parsing_num = false;

			bool ignore = false;

			byte* number = 0; size_t number_size = 0;
			
			uintmax_t radix = 10; bool signable = true;

			int equals_digits = 0;

			int max_digits = 0;

			if (format[temp_i] == '*') {
				ignore = true;

				temp_i += 1;
			}

			if (isnum(format[temp_i])) {
				size_t len = 0;

				parse_num(format + temp_i, 10, &max_digits, &len);

				temp_i += len;
			}

			else if (format[temp_i] == '*') {
				max_digits = va_arg(list, int);

				temp_i += 1;
			}

			if (format[temp_i] == '.') {
				if (isnum(format[temp_i + 1])) {
					size_t len = 0;

					parse_num(format + temp_i + 1, 10, &equals_digits, &len);

					temp_i += len + 1;
				}

				else if (format[temp_i + 1] == '*') {
					equals_digits = va_arg(list, int);

					temp_i += 2;
				}
			}

			if (format[temp_i] == 'l') {
				if (!parsing_num) {
					if (!ignore) {
						number = va_arg(list, long long*);
						number_size = sizeof(long long);
					}
					
					radix = 10; signable = true;
				}
				
				parsing_num = true;

				temp_i += 1;

				passed = true;
			}

			else if (format[temp_i] == 'z') {
				if (!parsing_num) {
					if (!ignore) {
						number = va_arg(list, size_t*);
						number_size = sizeof(size_t);
					}
					
					radix = 10; signable = false;
				}
				
				parsing_num = true;

				temp_i += 1;

				passed = true;
			}

			if (format[temp_i] == 'i') {
				if (!parsing_num) {
					if (!ignore) {
						number = va_arg(list, int*);
						number_size = sizeof(int);
					}
					
					radix = 10;
				}
					
				parsing_num = true;
				
				signable = true;

				temp_i += 1;

				passed = true;
			}

			else if (format[temp_i] == 'u') {
				if (!parsing_num) {
					if (!ignore) {
						number = va_arg(list, unsigned int*);
						number_size = sizeof(unsigned int);
					}

					radix = 10;
					
					parsing_num = true;
				}

				signable = false;

				temp_i += 1;

				passed = true;
			}

			if (format[temp_i] == 'x') {
				if (!parsing_num) {
					if (!ignore) {
						number = va_arg(list, int*);
						number_size = sizeof(int);
					}
					
					signable = false;
					
					parsing_num = true;
				}

				radix = 16;

				temp_i += 1;

				passed = true;
			}

			else if (format[temp_i] == 'd') {
				if (!parsing_num) {
					if (!ignore) {
						number = va_arg(list, int*);
						number_size = sizeof(int);
					}
					
					signable = true;
					
					parsing_num = true;
				}

				radix = 10;

				temp_i += 1;

				passed = true;
			}

			else if (format[temp_i] == 'b') {
				if (!parsing_num) {
					if (!ignore) {
						number = va_arg(list, int*);
						number_size = sizeof(int);
					}
					
					signable = false;
					
					parsing_num = true;
				}

				radix = 2;

				temp_i += 1;

				passed = true;
			}

			else if (format[temp_i] == 'o') {
				if (!parsing_num) {
					if (!ignore) {
						number = va_arg(list, int*);
						number_size = sizeof(int);
					}
					
					signable = false;
					
					parsing_num = true;
				}

				radix = 8;

				temp_i += 1;

				passed = true;
			}

			// if (format[temp_i] == 'n') {
			
			if (parsing_num) {
				uintmax_t result_num = 0;
				
				size_t len = 0;
					
				if (!ignore) {
					int err = nparse_num(s + r_index, max_digits, radix, &result_num, &len);

					if (err != PARSE_NUM_OK || len <= 0) {
						va_end(list);

						return writed_args;
					}

					if (equals_digits > 0 &&
						len != equals_digits) {
						va_end(list);

						return writed_args;
					}

					for (size_t j = 0; j < number_size; j++) {
						number[j] = result_num & 0xFF;

						result_num >>= 8;
					}
					
					writed_args += 1;
				}

				else {
					nparse_num(s + r_index, max_digits, radix, nullptr, &len);
				}

				r_index += len;

				passed = true;
			}

			else if (format[temp_i] == 's') {
				byte* str = va_arg(list, byte*);

				size_t w_index = 0;

				byte inputed_c = s[r_index++];

				while (isprintable(inputed_c)) {
					str[w_index++] = inputed_c;

					inputed_c = s[r_index++];
				}

				writed_args += 1;

				temp_i += 1;

				writed_args += 1;

				passed = true;
			}

			else if (format[temp_i] == '[') {
				temp_i += 1;

				bool inverse = false; 

				byte selected_chars[256] = { 0 }; size_t ch_pos = 0;

				byte* results = nullptr;

				if (!ignore) {
					results = va_arg(list, byte*);
				}

				if (format[temp_i] == '^') {
					inverse = true;

					temp_i += 1;
				}
					
				while (format[temp_i] != ']') {
					byte st_ch = format[temp_i];
					byte mid_ch = format[temp_i + 1];
					byte end_ch = format[temp_i + 2];

					if (st_ch <= end_ch && mid_ch == '-') {
						for (byte j = st_ch; j <= end_ch; j++) {
							selected_chars[ch_pos++] = j;
						}

						temp_i += 2;
					}

					else {
						for (; format[temp_i] && format[temp_i] != ']'; temp_i++) {
							selected_chars[ch_pos++] = format[temp_i];
						}

						if (!format[temp_i]) {
							va_end(list);

							return writed_args;
						}
					}

					temp_i += 1;
				}

				if (format[temp_i] == ']') temp_i += 1;

				print_hex(selected_chars, 0, 128); kprint("\n\r");

				byte inputed_c = s[r_index];

				while (isprintable(inputed_c)) {
					bool allow = inverse;

					for (size_t j = 0; selected_chars[j] && j < 256; j++) {
						if (inputed_c == selected_chars[j]) {
							allow = !inverse; break;
						}
					}

					if (allow) {
						if (results) {
							*(results++) = inputed_c;
						}
						
						r_index++;
					}

					else break;
					
					inputed_c = s[r_index];
				}

				if (results) writed_args++;

				passed = true;
			}

			if (!passed) break;

			i = temp_i;

			c = format[i];
		}

		if (c == '\0') {
			break;
		}

		else if (c == ' ') {
			byte inputed_c = s[r_index];

			while (!isprintable(inputed_c)) {
				inputed_c = s[r_index];

				r_index++;
			}
		}

		else {
			byte inputed_c = s[r_index++];

			if (inputed_c != c) break;
		}
	}

	va_end(list);

	return writed_args;
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

bool isupper(byte c) {
	return (c >= 'A') && (c <= 'Z');
}

bool islower(byte c) {
	return (c >= 'a') && (c <= 'z');
}

bool isnum(byte c) {
	return (c >= '0') && (c <= '9');
}

bool isalnum(byte c) {
	return isalpha(c) || isnum(c);
}

bool isalpha(byte c) {
	return islower(c) || isupper(c);
}

bool isascii(byte c) {
	return (c >= ' ') && (c <= 126);
}

bool isprintable(byte c) {
	return (c >= '!') && (c <= 254);
}

byte upper(byte c) {
	return islower(c) ? (c - 'a' + 'A') : c;
}

byte lower(byte c) {
	return isupper(c) ? (c - 'A' + 'a') : c;
}

static byte* str = nullptr; static size_t str_len = 0;

static ssize_t index = 0;

byte* strtok(byte* _str, const byte* delim) {
	if (!_str) {
		if (!str) return nullptr;

		if (index < 0) return nullptr;

		for (size_t i = index + 1; i < str_len; i++) {
			if (str[i] == 0) {
				byte* result = str + i + 1;

				index = i;

				return result;
			}
		}

		index = 0;
		
		return nullptr;
	}

	str = _str;
	
	size_t delim_len = strlen(delim);

	str_len = strlen(_str);

	for (size_t i = 0; _str[i]; i++) {
		size_t min_size = MIN(delim_len, str_len - 1);

		if (strncmp(_str + i, delim, min_size) == 0) {
			memset(_str + i, 0, min_size);

			i += min_size - 1;
		}
	}

	return _str;
}

byte* parse_cli_args(byte* _str) {
	if (!_str) {
		if (!str) return nullptr;

		if (index < 0) return nullptr;

		for (size_t i = index + 1; i < str_len; i++) {
			if (str[i] == 0) {
				for (; i < str_len; i++) {
					if (str[i] != 0) break;
				}

				byte* result = str + i;

				index = i;

				return result;
			}
		}

		index = 0;
		
		return nullptr;
	}

	str_len = strlen(_str);

	str = _str;

	bool quotes = false;
	bool double_quotes = false;

	bool escape = false;

	for (size_t i = 0; _str[i]; i++) {
		if (_str[i] == '"') {
			if (!escape) {
				double_quotes = !double_quotes;

				_str[i] = 0;
			}

			else escape = false;
		}

		if (_str[i] == '\'') {
			if (!escape) {
				quotes = !quotes;

				_str[i] = 0;
			}

			else escape = false;
		}

		if (_str[i] == '\\') {
			escape = true;

			size_t k = i;

			for (size_t j = i; _str[j] != ' ' && j < str_len; j++) {
				if (_str[j] != '\\') {
					_str[k] = _str[j];

					k++;
				}
			}

			for (size_t j = k; _str[j] != ' ' && j < str_len; j++) {
				_str[j] = ' ';
			}
		}

		if (_str[i] == ' ' && !quotes && !double_quotes) {
			_str[i] = 0;
		}
	}

	return _str;
}

void strip_str(byte* str, size_t size) {
	for (size_t i = size; i >= 0 && str[i] == ' '; i--) {
		if (str[i] != ' ') break;

		str[i] = 0;
	}
}

static const c_str num_alphabet = 
"0123456789"
"ABCDEFGHIJ"
"KLMNOPQRST"
"UVWXYZ";

static ssize_t ch_index_in_alphabet(char c, const c_str alphabet, size_t alphabet_size) {
	if (alphabet == nullptr || alphabet_size == 0) {
		kprint("Check char in alphabet failure: "
				"alphabet is nullptr or alphabet_size = 0\n");
		
		return -1;
	}

	for (size_t i = 0; i < alphabet_size; i++) {
		if (c == alphabet[i]) return i;
	}

	return -1;
}

ErrorCode parse_hex(byte* result, size_t res_size, const c_str str) {
	if (str == nullptr) {
		kprint("Hex parse failure: str is nullptr\n"); return CODE_FAIL;
	}

	if (result == nullptr) {
		kprint("Hex parse failure: result is nullptr\n"); return CODE_FAIL;
	}

	size_t digit_cnt = 0;

	while (ch_index_in_alphabet(upper(str[digit_cnt]), num_alphabet, 16) != -1) {
		result[digit_cnt / 2] = 0;
		
		digit_cnt++;
	}

	res_size *= 2;

	size_t bit_index = digit_cnt * 4;

	for (size_t i = 0; i < digit_cnt; i++) {
		bit_index -= 4;

		size_t byte_index = bit_index / 8;

		if (byte_index >= res_size) continue;

		char c = str[i];

		ssize_t index = ch_index_in_alphabet(upper(str[i]), num_alphabet, 16);

		if (index == -1) break;

		result[byte_index] += index << (bit_index % 8);
	}

	return CODE_OK;
}

int parse_num(const c_str str, uintmax_t radix, uintmax_t* _result, size_t* len) {
	if (str == nullptr) {
		return PARSE_NUM_INVLD_STR;
	}

	if (radix <= 1 || radix >= strlen(num_alphabet)) {
		return PARSE_NUM_INVLD_RADIX;
	}

	uintmax_t result = 0;

	size_t digit_cnt = 0;

	while (ch_index_in_alphabet(upper(str[digit_cnt]), num_alphabet, radix) != -1)
		digit_cnt++;
	
	if (digit_cnt == 0)
		return PARSE_NUM_INVLD_NUM;

	for (size_t i = 0; i < digit_cnt; i++) {
		byte c = upper(str[i]);

		size_t index = ch_index_in_alphabet(c, num_alphabet, radix);

		if (index == -1) {
			if (len) *len = digit_cnt;

			return PARSE_NUM_INVLD_NUM;
		}

		result *= radix;
		
		result += index;
	}

	if (_result) *_result = result;

	if (len) *len = digit_cnt;

	return PARSE_NUM_OK;
}

int nparse_num(const c_str str, ssize_t max_size, uintmax_t radix, uintmax_t* _result, size_t* len) {
	if (str == nullptr) {
		return PARSE_NUM_INVLD_STR;
	}

	if (radix <= 1 || radix >= strlen(num_alphabet)) {
		return PARSE_NUM_INVLD_RADIX;
	}

	uintmax_t result = 0;

	size_t digit_cnt = 0;

	while ((max_size <= 0 || digit_cnt < max_size) &&
			ch_index_in_alphabet(upper(str[digit_cnt]), num_alphabet, radix) != -1)
		digit_cnt++;

	for (size_t i = 0; i < digit_cnt; i++) {
		byte c = upper(str[i]);

		size_t index = ch_index_in_alphabet(c, num_alphabet, radix);

		if (index == -1) break;

		result *= radix;
		
		result += index;
	}

	if (_result) *_result = result;

	if (len) *len = digit_cnt;

	return PARSE_NUM_OK;
}

void snprint_hex(byte* s, ssize_t max_size, byte* num, size_t size) {
	if (!s) return;

	if (num == nullptr) return;

	if (max_size == 0)
		return;

	if (max_size < 0)
		max_size = strlen(s);

	size_t w_index = 0;

	for (size_t i = 0; i < size && w_index < max_size; i++) {
		byte hi = (num[i] >> 4) & 0xF,
			 lo = num[i] & 0xF;

		s[w_index++] = num_alphabet[hi];
		s[w_index++] = num_alphabet[lo];

		s[w_index++] = ' ';
	}
}

void print_hex(byte* num, size_t offset, size_t size) {
	if (num == nullptr) return;

	size_t i = 0;

	size_t pos_x = 0;

	disable_blink();

	size_t step = 3;

	byte ascii_buf[] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };

	size_t digits = MAX(5, get_num_digits(offset + size, 16, false) + 1);

	ssize_t width = get_columns() - (1 + (digits + 3) * 2 + sizeof(ascii_buf));

	size_t ascii_buf_size = MIN(width / 3, sizeof(ascii_buf));

	width = align_down(width, step) - (step * 2);

	kprintf("┌%0m─*s┬%0m─*s┬%0m─*s┬%0m─*s┐\n\r", digits, "", width, "", digits, "", ascii_buf_size, "");

	kprintf("│START│%=*s│%=*s│%=*s│\n\r", width, "HEX VIEW", digits, "END", ascii_buf_size, "ASCII");

	kprintf("├%0m─*s┼%0m─*s┼%0m─*s┼%0m─*s┤\n\r", digits, "", width, "", digits, "", ascii_buf_size, "");

	while (i < size) {
		set_style(COLOR_BRIGHT_WHITE);

		kprintf("│%vfw%.*lx%vd│", digits, MIN(size + offset, i + offset));

		size_t ascii_colon_width = MIN(sizeof(ascii_buf), width / step);

		#define READ_NUM(index) ((index) < (size) ? (num)[(index)] : 0)

		memset(ascii_buf, 0, sizeof(ascii_buf));

		size_t last_i = i;

		pos_x = 0;
		
		while (pos_x < width) {
			byte num_byte = READ_NUM(i);

			byte hi = (num_byte >> 4) & 0xF,
				lo = num_byte & 0xF;
			
			size_t color = (size_t)(hi + lo) / 2;
			
			if (hi == 0 && lo == 0) {
				set_style(COLOR_WHITE);
			}

			else {
				set_style(color >= 1 ? color : COLOR_BRIGHT_WHITE);
			}

			if (i >= size) {
				putch(' '); 
				putch(' '); 
				putch(' ');
			}

			else {
				putch(num_alphabet[hi]);
				putch(num_alphabet[lo]);

				putch(' ');
			}

			pos_x += step;

			i += 1;
		}

		set_style(COLOR_BRIGHT_WHITE);

		kprintf("│%vfw%.*lx%vd│", digits, MIN(size + offset, i + offset));

		for (size_t k = 0; k < ascii_buf_size; k++) {
			if (k >= (i - last_i)) {
				putch(' '); continue;
			}

			byte num_byte = READ_NUM(last_i + k);

			byte hi = (num_byte >> 4) & 0xF,
				lo = num_byte & 0xF;
			
			size_t color = (size_t)(hi + lo) / 2;
			
			if (hi == 0 && lo == 0) {
				set_style(COLOR_WHITE);
			}

			else {
				set_style(color >= 1 ? color : COLOR_BRIGHT_WHITE);
			}

			if (isprintable(num_byte))
				putch(num_byte);
			else
				putch('.');
		}

		set_style(COLOR_BRIGHT_WHITE);

		kprint("│\n\r");
	}

	kprintf("└%0m─*s┴%0m─*s┴%0m─*s┴%0m─*s┘", digits, "", width, "", digits, "", ascii_buf_size, "");

	crt_set_cursor_pos(cur_x, cur_y);
}

static size_t snprint_unum(byte* s, size_t max_size, uintmax_t num, uintmax_t base) {
	if (base <= 0 || base >= strlen(num_alphabet)) {
		kprint("Base not in range 2...37\n");

		return 0;
	}

	if (max_size == 0)
		return 0;

	if (max_size < 0)
		max_size = strlen(s);

	if (num < base) {
		s[0] = num_alphabet[num];

		return 1;
	}

	uintmax_t digits = get_num_digits(num, base, false);

	ssize_t index = digits;

	while (num >= base && index > 0) {
		if (index < max_size)
			s[index] = num_alphabet[num % base];

		num /= base;

		index -= 1;
	}
	
	s[0] = num_alphabet[num % base];

	return MIN(max_size, digits + 1);
}

size_t snprint_num(byte* s, ssize_t max_size, size_t num, size_t base, bool num_signed, bool always_show_sign) {
	if (base <= 0 || base >= strlen(num_alphabet)) {
		return 0;
	}

	size_t result = 0;

	if (num_signed) {
		size_t mask = 1ULL << ((sizeof(num) * 8ULL) - 1ULL);

		bool sign = num & mask;
		
		if (sign) {
			s[0] = '-';

			result += 1;
		}

		else if (always_show_sign) {
			s[0] = '+';

			result += 1;
		}
		
		num = (~num) + 1; // Converting to positive number to negative, and vice versa
	}

	return result + snprint_unum(s + result, max_size - result, num, base);
}

size_t print_num(uintmax_t num, uintmax_t base, bool num_signed, bool always_show_sign) {
	if (base <= 0 || base >= strlen(num_alphabet)) {
		return 0;
	}

	size_t result = 0;

	if (num_signed) {
		uintmax_t mask = 1ULL << ((sizeof(num) * 8ULL) - 1ULL);

		bool sign = num & mask;
		
		if (sign) {
			putch('-');

			result += 1;
		
			num = (~num) + 1; // Converting to positive number to negative, and vice versa
		}

		else if (always_show_sign) {
			putch('+');

			result += 1;
		}
	}

	byte buf[64] = { 0 };

	result += snprint_unum(buf, 64, num, base);

	kprint(buf);

	crt_set_cursor_pos(cur_x, cur_y);

	return result;
}

void set_cursor_pos(ssize_t _x, ssize_t _y) {
	size_t columns = get_columns();

	ssize_t pos = _x + (_y * columns);

	ssize_t x = columns > 0 ? pos % columns : 0;
	ssize_t y = columns > 0 ? pos / columns : 0;

	cur_x = x; cur_y = y;
}

void get_cursor_pos(ssize_t* x, ssize_t* y) {
	if (x) *x = cur_x;

	if (y) *y = cur_y;
}
