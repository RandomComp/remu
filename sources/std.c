#include "std.h"

#include "types.h"

#include "colors.h"

#include "builtins/string.h"

#include "drivers/video/vga.h"

#include "drivers/hid/kbdps2.h"

#include "builtins/builtins.h"

#include "math/math.h"

#include "kernel.h"

#define vidmem_size (COLUMNS * STD_ROWS)

static uint16* vidmem = nullptr;

static byte cur_style = 0x0F;

static ssize_t cur_x = 0, cur_y = 0;

void init_std(uint16* _vidmem) {
	cur_style = 0x0F;

	cur_x = 0, cur_y = 0;

	vidmem = _vidmem;
}

void clear_screen(byte style) {
	if (!vidmem) return;

	set_cursor_pos(0, 0);

	for (size_t i = 0; i < VGA_VIDMEM_SIZE; i++) {
		vidmem[i] = (uint16)style << 8;
	}
}

void putch(byte c) {
	if (!vidmem) return;

	ssize_t cur_pos = (cur_y * COLUMNS) + cur_x;
	
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
				vidmem[cur_pos] = cur_style << 8 | c;

			cur_x += 1;
	}

	cur_pos = CLAMP((cur_y * COLUMNS) + cur_x, 0, COLUMNS * ROWS);

	set_cursor_pos(cur_pos % COLUMNS, cur_pos / COLUMNS);

	while (cur_y >= ROWS) {
		for (size_t i = 0; i < ROWS - 1; i++) {
			size_t next_i = i + 1;

			memcpy(vidmem + (i * COLUMNS), vidmem + (next_i * COLUMNS), COLUMNS * 2);
		}

		memset(vidmem + ((ROWS - 1) * COLUMNS), 0, COLUMNS * 2);

		cur_y--;
	}
}

void clear_line() {
	ssize_t old_x = cur_x, old_y = cur_y;

	for (size_t i = 0; i < (COLUMNS - cur_x); i++) {
		putch(' ');
	}

	set_cursor_pos(old_x, old_y);
}

size_t kprint(const c_str str) {
	if (!vidmem) return 0;

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
	if (!vidmem) return 0;

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

size_t vsnprintf(byte* s, ssize_t max_len, const c_str format, va_list list) {
	ssize_t w_index = 0;

	for (size_t i = 0; format[i]; i++) {
		byte* c = format + i;

		while (*c == '%') {
			bool handled = false;

			size_t temp_i = i + 1;

			uintmax_t alignment = 0;
			bool left_alignment = true; bool right_alignment = false; bool center_alignment = false;

			int min_digits = 0;

			byte c_filler = ' ';

			if (format[temp_i] == '0') {
				if (format[temp_i + 1] == 'm') {
					c_filler = format[temp_i + 2];

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

				temp_i += len + 1;
				
				handled = true;
			}

			else if (format[temp_i] == '*') {
				alignment = va_arg(list, int);

				temp_i += 1;
				
				handled = true;
			}
			
			if (format[temp_i] == '.') {
				if (isnum(format[temp_i + 1])) {
					size_t len = 0;
					
					parse_num(format + temp_i + 1, 10, &min_digits, &len);

					min_digits = min_digits - 1;

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
				num = va_arg(list, uint64); signable = false; radix = 10;

				temp_i += 1;
				
				handled = true;
			}

			if (format[temp_i] == 'z') {
				printing_num = true;
				num = (uintmax_t)va_arg(list, size_t); signable = false; radix = 10;

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
				
				handled = true;
			}
			
			else if (format[temp_i] == 'd') {
				if (!printing_num) {
					num = va_arg(list, int); signable = true;
				}

				printing_num = true; radix = 10;

				temp_i += 1;
				
				handled = true;
			}
			
			else if (format[temp_i] == 'x') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;
				}

				printing_num = true; radix = 16;

				temp_i += 1;
				
				handled = true;
			}
			
			else if (format[temp_i] == 'b') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;
				}

				printing_num = true; radix = 2;

				temp_i += 1;
				
				handled = true;
			}

			else if (format[temp_i] == 'o') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;
				}

				printing_num = true; radix = 8;

				temp_i += 1;
				
				handled = true;
			}

			if (printing_num) {
				ssize_t digits = get_num_digits(num, radix, signable);

				if ((min_digits - digits) > 0) {
					for (ssize_t j = 0; j < (min_digits - digits); j++) {
						s[w_index++] = '0';

						if (w_index >= max_len) {
							return w_index;
						}
					}
				}

				w_index += snprint_num(s + w_index, max_len - w_index, num, radix, signable, always_print_sign);

				if (w_index >= max_len) {
					return w_index;
				}

				handled = true;
			}

			else if (format[temp_i] == 's') {
				byte* str = va_arg(list, c_str);

				size_t str_len = strlen(str);

				if (right_alignment) {
					if (alignment > str_len) {
						for (ssize_t j = 0; j < alignment - str_len; j++) {
							s[w_index++] = c_filler;

							if (w_index >= max_len) {
								return w_index;
							}
						}
					}
				}

				else if (center_alignment) {
					if (alignment > str_len) {
						for (ssize_t j = 0; j < align_down(alignment, 2) / 2 - align_down(str_len, 2) / 2; j++) {
							s[w_index++] = c_filler;

							if (w_index >= max_len) {
								return w_index;
							}
						}
					}
				}

				min_digits = min_digits == 0 ? str_len : min_digits;

				for (size_t j = 0; str[j] && j < min_digits; j++) {
					if (str[j] == '\n') {
						s[w_index++] = '\n';
						
						s[w_index++] = '\r';

						if (w_index >= max_len) {
							return w_index;
						}
					}
					
					else s[w_index++] = str[j];
				}

				if (w_index >= max_len) {
					return w_index;
				}

				if (left_alignment) {
					if (alignment > str_len) {
						for (ssize_t j = 0; j < alignment - str_len; j++) {
							s[w_index++] = c_filler;

							if (w_index >= max_len) {
								return w_index;
							}
						}
					}
				}

				else if (center_alignment) {
					if (alignment > str_len) {
						for (ssize_t j = 0; j < align_up(alignment, 2) / 2 - align_up(str_len, 2) / 2; j++) {
							s[w_index++] = c_filler;

							if (w_index >= max_len) {
								return w_index;
							}
						}
					}
				}

				temp_i += 1;
				
				handled = true;
			}

			else if (format[temp_i] == 'c') {
				s[w_index++] = va_arg(list, int);

				if (w_index >= max_len) {
					return w_index;
				}

				temp_i += 1;

				handled = true;
			}

			if (!handled) break;

			i = temp_i;

			c = format + i;
		}

		c = format + i;

		if (*c == '\n') {
			s[w_index++] = '\n';
			s[w_index++] = '\r';

			if (w_index >= max_len) {
				return w_index;
			}
		}

		else if (*c == '\0') {
			s[w_index++] = *c;

			if (w_index >= max_len) {
				return w_index;
			}
			
			break;
		}
		
		else {
			s[w_index++] = *c;

			if (w_index >= max_len) {
				return w_index;
			}
		}
	}

	return w_index;
}

size_t snprintf(byte *c, ssize_t max_len, const c_str format, ...) {
	va_list list;

	va_start(list, format);

	size_t writed = vsnprintf(c, max_len, format, list);

	va_end(list);

	return writed;
}

size_t vsprintf(byte* c, const c_str format, va_list list) {
	return vsnprintf(c, -1, format, list);
}

size_t sprintf(byte* c, const c_str format, ...) {
	va_list list;

	va_start(list, format);

	size_t writed = vsnprintf(c, -1, format, list);

	va_end(list);

	return writed;
}

size_t kprintf(const c_str format, ...) {
	va_list list;

	va_start(list, format);
	
	byte def_style = cur_style;

	size_t w_index = 0;

	for (size_t i = 0; format[i]; i++) {
		byte* c = format + i;

		while (*c == '%') {
			size_t temp_i = i + 1;

			bool passed = false;

			intmax_t alignment = 0;
			bool left_alignment = true; bool right_alignment = false; bool center_alignment = false;

			intmax_t min_digits = 0;

			byte c_filler = ' ';

			if (format[temp_i] == '0') {
				if (format[temp_i + 1] == 'm') {
					c_filler = format[temp_i + 2];

					temp_i += 2;
				}

				else {
					c_filler = '0';
				}

				temp_i += 1;
			}

			if (format[temp_i] == '-') {
				left_alignment = false; right_alignment = true; center_alignment = false;
				
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
			
			if (format[temp_i] == '.') {
				if (isnum(format[temp_i + 1])) {
					size_t len = 0;
					
					parse_num(format + temp_i + 1, 10, &min_digits, &len);

					min_digits = min_digits - 1;

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
					num = va_arg(list, unsigned int);
				}
				
				printing_num = true;

				signable = false; radix = 10;

				temp_i += 1;
				
				passed = true;
			}
			
			if (format[temp_i] == 'i') {
				if (!printing_num) {
					num = va_arg(list, int);
				}
				
				printing_num = true;

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
				}

				printing_num = true; radix = va_arg(list, unsigned int);

				if (radix < 2 || radix > 32)
					radix = 10;

				temp_i += 1;
				
				passed = true;
			}
			
			else if (format[temp_i] == 'd') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;
				}

				printing_num = true; radix = 10;

				temp_i += 1;
				
				passed = true;
			}
			
			else if (format[temp_i] == 'x') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;
				}

				printing_num = true; radix = 16;

				temp_i += 1;
				
				passed = true;
			}
			
			else if (format[temp_i] == 'b') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;
				}

				printing_num = true; radix = 2;

				temp_i += 1;
				
				passed = true;
			}
			
			else if (format[temp_i] == 'o') {
				if (!printing_num) {
					num = va_arg(list, unsigned int); signable = false;
				}

				printing_num = true; radix = 8;

				temp_i += 1;
				
				passed = true;
			}

			if (printing_num) {
				ssize_t digits = get_num_digits(num, radix, signable);

				if ((min_digits - digits) > 0) {
					for (ssize_t j = 0; j < (min_digits - digits); j++) {
						putch('0');
					}

					w_index += min_digits - digits;
				}

				w_index += print_num(num, radix, signable, always_print_sign);
				
				passed = true;
			}

			else if (format[temp_i] == 's') {
				byte* str = va_arg(list, c_str);

				size_t str_len = strlen(str);

				min_digits = min_digits == 0 ? str_len : min_digits;

				str_len = MIN(min_digits, str_len);

				if (right_alignment) {
					if (alignment > str_len) {
						for (ssize_t j = 0; j < alignment - str_len; j++) {
							putch(c_filler);
						}
					}
				}

				else if (center_alignment) {
					intmax_t cnt = align_down(alignment, 2) / 2 - align_down(str_len, 2) / 2;

					if (cnt > 0) {
						for (ssize_t j = 0; j < cnt; j++) {
							putch(c_filler);
						}
					}
				}

				for (int j = 0; str[j] && j <= min_digits; j++) {
					if (str[j] == '\n') {
						putch('\n'); putch('\r');
					}
					
					else putch(str[j]);
				}

				if (left_alignment) {
					if (alignment > str_len) {
						for (ssize_t j = 0; j < alignment - str_len; j++) {
							putch(c_filler);
						}
					}
				}

				else if (center_alignment) {
					intmax_t cnt = align_up(alignment, 2) / 2 - align_up(str_len, 2) / 2;

					if (cnt > 0) {
						for (intmax_t j = 0; j < cnt; j++) {
							putch(c_filler);
						}
					}
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

byte blkgetch(void) {
	byte result = 0;

	while (!result) {
		result = getch();

		halt();
	}

	return result;
}

size_t sscanf(const byte* s, const c_str format, ...) {
	va_list list;

	va_start(list, format);
	
	size_t writed_args = 0;

	size_t r_index = 0;

	for (size_t i = 0; format[i]; i++) {
		byte c = format[i];

		while (!isprintable(c)) {
			i++;

			c = format[i];
		}

		while (c == '%') {
			size_t temp_i = i + 1;

			bool passed = false;

			bool parsing_num = false;

			byte* number = 0; size_t number_size = 0;
			
			uintmax_t radix = 10; bool signable = true;

			if (format[temp_i] == 'l') {
				if (!parsing_num) {
					number = va_arg(list, long long*);
					number_size = sizeof(long long);
					
					radix = 10; signable = true;
				}
				
				parsing_num = true;

				temp_i += 1;

				passed = true;
			}

			else if (format[temp_i] == 'z') {
				if (!parsing_num) {
					number = va_arg(list, size_t*);
					number_size = sizeof(size_t);
					
					radix = 10; signable = false;
				}
				
				parsing_num = true;

				temp_i += 1;

				passed = true;
			}

			if (format[temp_i] == 'i') {
				if (!parsing_num) {
					number = va_arg(list, int*);
					number_size = sizeof(int);
					
					radix = 10;
				}
					
				parsing_num = true;
					
				signable = true;

				temp_i += 1;

				passed = true;
			}

			else if (format[temp_i] == 'u') {
				if (!parsing_num) {
					number = va_arg(list, unsigned int*);
					number_size = sizeof(unsigned int);

					radix = 10;
				}

				parsing_num = true;

				signable = false;

				temp_i += 1;

				passed = true;
			}

			if (parsing_num) {
				uintmax_t result_num = 0;
				
				size_t len = 0;
					
				parse_num(s + r_index, radix, &result_num, &len);

				r_index += len;

				for (size_t j = 0; j < number_size; j++) {
					number[j] = result_num & 0xFF;

					result_num >>= 8;
				}

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

				temp_i += 1;

				writed_args += 1;

				passed = true;
			}

			if (!passed) break;

			i = temp_i;

			c = format[i];
		}

		if (c == '\0') {
			break;
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
	if (!_result) {
		return PARSE_NUM_INVLD_RESULT_PTR;
	}

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

	for (size_t i = 0; i < digit_cnt; i++) {
		byte c = upper(str[i]);

		size_t index = ch_index_in_alphabet(c, num_alphabet, radix);

		if (index == -1) break;

		result *= radix;
		
		result += index;
	}

	*_result = result;

	if (len) *len = digit_cnt;

	return PARSE_NUM_OK;
}

void snprint_hex(byte* s, ssize_t max_size, byte* num, size_t size) {
	if (!vidmem) return;

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
	if (!vidmem) return;

	if (num == nullptr) return;

	size_t i = 0;

	size_t pos_x = 0;

	disable_blink();

	size_t step = 3;

	byte ascii_buf[] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };

	size_t digits = MAX(4, get_num_digits(offset + size, 16, false) + 1);

	ssize_t width = COLUMNS - (1 + (digits + 3) * 2 + sizeof(ascii_buf));

	width = align_down(width, step) - (step * 2);

	kprintf("�%0m�*s�%0m�*s�%0m�*s�%0m�*s�\n\r", digits + 1, "", width, "", digits + 1, "", sizeof(ascii_buf), "");

	kprintf("�START�%=*s�%=*s�%=*s�\n\r", width, "HEX VIEW", digits + 1, "END", sizeof(ascii_buf), "ASCII");

	kprintf("�%0m�*s�%0m�*s�%0m�*s�%0m�*s�\n\r", 5, "", width, "", digits + 1, "", sizeof(ascii_buf), "");

	while (i < size) {
		set_style(COLOR_BRIGHT_WHITE);

		kprintf("�%vfw%.*lx%vd�", digits, MIN(size + offset, i + offset));

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

		kprintf("�%vfw%.*lx%vd�", digits, MIN(size + offset, i + offset));

		for (size_t k = 0; k < sizeof(ascii_buf); k++) {
			if (k >= (i - last_i)) {
				putch('.'); continue;
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

			if (isascii(num_byte))
				putch(num_byte);
			else
				putch('.');
		}

		set_style(COLOR_BRIGHT_WHITE);

		kprint("�\n\r");
	}

	kprintf("�%0m�*s�%0m�*s�%0m�*s�%0m�*s�", digits + 1, "", width, "", digits + 1, "", sizeof(ascii_buf), "");

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
	if (!vidmem) return 0;

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
	ssize_t pos = _x + (_y * COLUMNS);

	ssize_t x = pos % COLUMNS;
	ssize_t y = pos / COLUMNS;

	cur_x = x; cur_y = y;

	// crt_set_cursor_pos(cur_x, cur_y);
}

void get_cursor_pos(ssize_t* x, ssize_t* y) {
	if (x) *x = cur_x;

	if (y) *y = cur_y;
}

void set_style(byte style) {
	cur_style = style;
}

byte get_style(void) {
	return cur_style;
}
