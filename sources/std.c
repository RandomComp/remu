#include "std.h"

#include "types.h"

#include "builtins/string.h"

#include "drivers/video/vga.h"

#include "math/math.h"

#define columns 80

#define rows 25

#define vidmem_size (columns * rows)

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

	for (size_t i = 0; i < vidmem_size; i++) {
		vidmem[i] = (uint16)style << 8;
	}
}

void putch(byte c) {
	if (!vidmem) return;

	ssize_t cur_pos = (cur_y * columns) + cur_x;
	
	switch (c) {
		case '\b':
			cur_x -= 1; break;
		
		case '\n':
			cur_y += 1; break;
		
		case '\r':
			cur_x = 0; break;
		
		default:
			if (cur_pos >= 0 && cur_pos < vidmem_size)
				vidmem[cur_pos] = cur_style << 8 | c;

			cur_x += 1;
	}

	cur_pos = CLAMP((cur_y * columns) + cur_x, 0, columns * rows);

	set_cursor_pos(cur_pos % columns, cur_pos / columns);

	while (cur_y >= rows) {
		for (size_t i = 0; i < rows - 1; i++) {
			size_t next_i = i + 1;

			memcpy(vidmem + (i * columns), vidmem + (next_i * columns), columns * 2);
		}

		memset(vidmem + ((rows - 1) * columns), 0, columns * 2);

		cur_y--;
	}
}

void kprint(const c_str str) {
	if (!vidmem) return;

	for (size_t i = 0; str[i]; i++) {
		putch(str[i]);
	}
}

void kprintf(const c_str format, ...) {
	if (!vidmem) return;

	va_list list;

	va_start(list, format);

	for (size_t i = 0; format[i]; i++) {
		char* c = format + i;

		if (*c == '%') {
			size_t temp_i = i + 1;
			
			bool padding = false; // false -- right padding, true -- left padding

			char padding_c = ' ';

			bool print_num_sign = false;

			if (format[temp_i] == '-') {
				padding = true;

				temp_i += 1;
			}

			else if (format[temp_i] == '+') {
				print_num_sign = true;

				temp_i += 1;
			}

			else if (format[temp_i] == '0') {
				padding_c = '0';

				temp_i += 1;
			}

			

			if (format[temp_i] == 'l') {
				print_num(va_arg(list, uint64), 10);

				temp_i += 1;
			}
			
			if (format[temp_i] == 'i') {
				print_num(va_arg(list, int), 10);

				temp_i += 1;
			}
			
			if (format[temp_i] == 'x' || format[temp_i] == 'x') {
				kprint("0x"); print_num(va_arg(list, uint32), 16);

				temp_i += 1;
			}
			
			if (format[temp_i] == 'b') {
				kprint("0b"); print_num(va_arg(list, int), 2);

				temp_i += 1;
			}

			if (format[temp_i] == 's') {
				kprint(va_arg(list, c_str));

				temp_i += 1;
			}

			if (format[temp_i] == 'c') {
				putch(va_arg(list, int));

				temp_i += 1;
			}

			i = temp_i;
		}

		c = format + i;

		if (*c == '\n') {
			putch('\n'); putch('\r');
		}

		else putch(*c);
	}

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

bool _isupper(char c) {
	return (c >= 'A' && c <= 'Z');
}

bool _islower(char c) {
	return (c >= 'a' && c <= 'z');
}

bool _isalpha(char c) {
	return _islower(c) || _isupper(c);
}

char upper(char c) {
	return _islower(c) ? (c - 'a' + 'A') : c;
}

char lower(char c) {
	return _isupper(c) ? (c - 'A' + 'a') : c;
}

const char num_alphabet[] = 
	"0123456789"
	"ABCDEFGHIJ"
	"KLMNOPQRST"
	"UVWXYZ";

static ssize_t ch_index_in_alphabet(char c, const c_str alphabet, size_t alphabet_size) {
	if (alphabet == nullptr || alphabet_size == 0) {
		kprint("Check char in alphabet failure: "
				"alphabet is nullptr or alphabet_size = 0\n\r");
		
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

void print_num(size_t num, size_t base) {
	if (!vidmem) return;

	if (base <= 0 || base >= strlen(num_alphabet)) {
		kprint("Base should be in range 2...");

		print_num(strlen(num_alphabet) - 1, 10);

		kprint(", not ");

		print_num(base, 10);

		kprint("\n\r");

		return;
	}

	if (num < 0) putch('-');

	num = ABS(num);

	if (num < base) {
		putch(num_alphabet[num]);

		return;
	}

	char buf[32] = { 0 };

	size_t buf_size = sizeof(buf) / sizeof(buf[0]);

	size_t index = MIN(buf_size - 1, get_num_digits(num, base));

	while (num >= base && index > 0) {
		buf[index] = num_alphabet[num % base];

		num /= base;

		index -= 1;
	}
	
	buf[index] = num_alphabet[num % base];

	kprint(buf);
}

void nblk__set_cursor_pos(ssize_t x, ssize_t y) {
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
