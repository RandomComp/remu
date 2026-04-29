#include "std.h"

#include "types.h"

#include "colors.h"

#include "builtins/string.h"

#include "drivers/video/vga.h"

#include "math/math.h"

#define vidmem_size (STD_COLUMNS * STD_ROWS)

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

	for (size_t i = 0; i < vidmem_size; i++) {
		vidmem[i] = (uint16)style << 8;
	}
}

void putch(byte c) {
	if (!vidmem) return;

	ssize_t cur_pos = (cur_y * STD_COLUMNS) + cur_x;
	
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
			if (cur_pos >= 0 && cur_pos < vidmem_size)
				vidmem[cur_pos] = cur_style << 8 | c;

			cur_x += 1;
	}

	cur_pos = CLAMP((cur_y * STD_COLUMNS) + cur_x, 0, STD_COLUMNS * STD_ROWS);

	set_cursor_pos(cur_pos % STD_COLUMNS, cur_pos / STD_COLUMNS);

	while (cur_y >= STD_ROWS) {
		for (size_t i = 0; i < STD_ROWS - 1; i++) {
			size_t next_i = i + 1;

			memcpy(vidmem + (i * STD_COLUMNS), vidmem + (next_i * STD_COLUMNS), STD_COLUMNS * 2);
		}

		memset(vidmem + ((STD_ROWS - 1) * STD_COLUMNS), 0, STD_COLUMNS * 2);

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

	return i;
}

size_t sprint(char *c, const c_str str) {
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

size_t parse_ext_specf(const char* format, byte def_style) {
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
		const char supported_colors[] = "augqrmyw";
		
		const byte colors_available[] = { 
			COLOR_BLACK,
			COLOR_BLUE,
			COLOR_GREEN,
			COLOR_AQUA,
			COLOR_RED,
			COLOR_MAGENTA,
			COLOR_BROWN,
			COLOR_WHITE,
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
			byte color = colors_available[index] | (brighted << 3);

			if (background)
				color = (color << 4) | (get_style() & 0xF);
			else if (foreground)
				color = color | ((get_style() >> 4) & 0xF);

			set_style(color);

			temp_i += 1;
		}
	}

	return temp_i;
}

size_t vsprintf(char* s, const c_str format, va_list list) {
	byte def_style = cur_style;

	size_t w_index = 0;

	for (size_t i = 0; format[i]; i++) {
		char* c = format + i;

		while (*c == '%') {
			size_t temp_i = i + 1;
			
			bool padding = false; // false -- right padding, true -- left padding

			char padding_c = ' ';

			bool print_num_sign = false;

			// if (format[temp_i] == '-') {
			// 	padding = true;

			// 	temp_i += 1;
			// }

			// else if (format[temp_i] == '+') {
			// 	print_num_sign = true;

			// 	temp_i += 1;
			// }

			// else if (format[temp_i] == '0') {
			// 	padding_c = '0';

			// 	temp_i += 1;
			// }

			

			if (format[temp_i] == 'l') {
				w_index += sprint_num(s, va_arg(list, uint64), 10, false, false);

				temp_i += 1;
			}
			
			else if (format[temp_i] == 'i') {
				int num = va_arg(list, int);

				w_index += sprint_num(s, num, 10, num < 0, false);

				temp_i += 1;
			}
			
			else if (format[temp_i] == 'x') {
				int num = va_arg(list, int);

				w_index += sprint_num(s, num, 16, num < 0, false);

				temp_i += 1;
			}
			
			else if (format[temp_i] == 'b') {
				int num = va_arg(list, int);

				w_index += sprint_num(s, num, 2, num < 0, false);

				temp_i += 1;
			}

			else if (format[temp_i] == 's') {
				w_index += sprint(s, va_arg(list, c_str));

				temp_i += 1;
			}

			else if (format[temp_i] == 'c') {
				s[w_index++] = va_arg(list, int);

				temp_i += 1;
			}

			i = temp_i;

			c = format + i;
		}

		c = format + i;

		if (*c == '\n') {
			s[w_index++] = '\n';
			
			s[w_index++] = '\r';
		}

		else if (*c == '\0') break;
		
		else s[w_index++] = *c;
	}

	return w_index;
}

size_t sprintf(char *c, const c_str format, ...) {
	va_list list;

	va_start(list, format);

	size_t writed = vsprintf(c, format, list);

	va_end(list);

	return writed;
}

size_t kprintf(const c_str format, ...) {
	va_list list;

	va_start(list, format);
	
	byte def_style = cur_style;

	size_t w_index = 0;

	for (size_t i = 0; format[i]; i++) {
		char* c = format + i;

		while (*c == '%') {
			size_t temp_i = i + 1;
			
			bool padding = false; // false -- right padding, true -- left padding

			char padding_c = ' ';

			bool print_num_sign = false;

			// if (format[temp_i] == '-') {
			// 	padding = true;

			// 	temp_i += 1;
			// }

			// else if (format[temp_i] == '+') {
			// 	print_num_sign = true;

			// 	temp_i += 1;
			// }

			// else if (format[temp_i] == '0') {
			// 	padding_c = '0';

			// 	temp_i += 1;
			// }

			

			if (format[temp_i] == 'l') {
				print_num(va_arg(list, uint64), 10, false);

				temp_i += 1;
			}
			
			else if (format[temp_i] == 'i') {
				int num = va_arg(list, int);

				print_num(num, 10, (num < 0));

				temp_i += 1;
			}
			
			else if (format[temp_i] == 'x') {
				int num = va_arg(list, int);

				print_num(num, 16, num < 0);

				temp_i += 1;
			}
			
			else if (format[temp_i] == 'b') {
				int num = va_arg(list, int);

				print_num(num, 2, num < 0);

				temp_i += 1;
			}

			else if (format[temp_i] == 's') {
				kprint(va_arg(list, c_str));

				temp_i += 1;
			}

			else if (format[temp_i] == 'c') {
				putch(va_arg(list, int));

				temp_i += 1;
			}

			else if (format[temp_i] == 'v') {
				temp_i += 1;

				temp_i += parse_ext_specf(format + temp_i, def_style);
			}

			i = temp_i;

			c = format + i;
		}

		c = format + i;

		if (*c == '\n') {
			putch('\n'); putch('\r');
		}

		else if (*c == '\0') break;
		
		else putch(*c);
	}
	
	set_style(def_style);

	va_end(list);
}

size_t get_num_digits(ssize_t num, size_t base) {
	size_t result = 0;

	while (num >= base) {
		result += 1;

		num /= base;
	}

	return MAX(result, 1);
}

bool isupper(byte c) {
	return (c >= 'A') && (c <= 'Z');
}

bool islower(byte c) {
	return (c >= 'a') && (c <= 'z');
}

bool isalpha(byte c) {
	return islower(c) || isupper(c);
}

bool isascii(byte c) {
	return (c >= ' ') && (c <= 128);
}

bool isprintable(byte c) {
	return (c >= ' ') && (c <= 255);
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

static const char num_alphabet[] = 
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

uintmax_t parse_num(const c_str str, uintmax_t radix) {
	if (str == nullptr) {
		kprint("Number parse failure: str is nullptr\n"); return CODE_FAIL;
	}

	if (radix <= 1 || radix >= strlen(num_alphabet)) {
		kprint("Number parse failure: radix would be in 2...35\n");

		return CODE_FAIL;
	}

	uintmax_t result = 0;

	size_t digit_cnt = 0;

	while (ch_index_in_alphabet(upper(str[digit_cnt]), num_alphabet, radix) != -1)
		digit_cnt++;

	for (size_t i = 0; i < digit_cnt; i++) {
		char c = upper(str[i]);

		size_t index = ch_index_in_alphabet(c, num_alphabet, radix);

		if (index == -1) break;

		result *= radix;
		
		result += index;
	}

	return result;
}

void sprint_hex(char* s, byte* num, size_t size) {
	if (!vidmem) return;

	if (num == nullptr) return;

	size_t w_index = 0;

	for (size_t i = 0; i < size; i++) {
		byte hi = (num[i] >> 4) & 0xF,
			 lo = num[i] & 0xF;

		s[w_index] = num_alphabet[hi]; 	w_index += 1;
		s[w_index] = num_alphabet[lo]; 	w_index += 1;

		s[w_index] = ' '; 				w_index += 1;
	}
}

void print_hex(byte* num, size_t size) {
	if (!vidmem) return;

	if (num == nullptr) return;

	for (size_t i = 0; i < size; i++) {
		byte hi = (num[i] >> 4) & 0xF,
			 lo = num[i] & 0xF;

		putch(num_alphabet[hi]);
		putch(num_alphabet[lo]);

		putch(' ');
	}
}

static size_t sprint_unum(char* s, size_t num, size_t base) {
	if (base <= 0 || base >= strlen(num_alphabet)) {
		kprint("Base not in range 2...37\n");

		return 0;
	}

	size_t w_index = 0;

	if (num < base) {
		s[w_index] = num_alphabet[num]; w_index++;

		return w_index;
	}

	size_t index = get_num_digits(num, base);

	while (num >= base && index > 0) {
		s[index + w_index] = num_alphabet[num % base];

		num /= base;

		index -= 1;
	}
	
	s[index + w_index] = num_alphabet[num % base];

	return index + w_index;
}

size_t sprint_num(char* s, size_t num, size_t base, bool num_signed, bool always_show_sign) {
	if (base <= 0 || base >= strlen(num_alphabet)) {
		return 0;
	}

	if (num_signed) {
		size_t mask = 1ULL << ((sizeof(num) * 8ULL) - 1ULL);

		bool sign = num & mask;
		
		if (sign) {
			s[0] = '-';

			s += 1;
		}

		else if (always_show_sign) {
			s[0] = '+';

			s += 1;
		}
		
		num = (~num) + 1; // Converting to positive number to negative, and vice versa
	}

	return sprint_unum(s, num, base);
}

void print_num(size_t num, size_t base, bool num_signed) {
	if (!vidmem) return;

	if (base <= 0 || base >= strlen(num_alphabet)) {
		return;
	}

	char buf[64] = { 0 };

	sprint_num(buf, num, base, num_signed, false);

	kprint(buf);
}

void set_cursor_pos(ssize_t x, ssize_t y) {
	cur_x = x; cur_y = y;

	crt_set_cursor_pos(cur_x, cur_y);
}

void get_cursor_pos(ssize_t* x, ssize_t* y) {
	if (x) *x = cur_x;

	if (y) *y = cur_y;
}

void set_style(byte style) {
	cur_style = style;
}

byte get_style() {
	return cur_style;
}
