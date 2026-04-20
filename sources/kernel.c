#include "blockstd.h"

#include "types.h"

#include "drivers/memory/memory.h"

#include "drivers/time/tsc.h"

#include "colors.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "math/math.h"

#include "drivers/power.h"

#include "drivers/time/cmos.h"

#include "bcd.h"

#include "time/time.h"

#define columns 80

#define rows 25

uint16* vidmem = nullptr;

#define vidmem_size (columns * rows)

byte cur_style = 0x0F;

_ssize_t cur_x = 0, cur_y = 0;

void clear_screen(byte style) {
	if (!vidmem) return;

	for (_size_t i = 0; i < vidmem_size; i++) {
		vidmem[i] = (uint16)style << 8;
	}
}

void putch(byte c) {
	if (!vidmem) return;

	_ssize_t cur_pos = (cur_y * columns) + cur_x;
	
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

	cur_pos = (cur_y * columns) + cur_x;
	
	cur_x = cur_pos % columns;

	cur_y = cur_pos / columns;

	if (cur_y >= rows) {
		for (_size_t i = 0; i < rows; i++) {
			_size_t next_row = MIN(rows - 1, i + 1);

			memcpy(vidmem + (i * columns), vidmem + (next_row * columns), columns);
		}

		cur_y--;
	}
}

void print(const c_str str) {
	if (!vidmem) return;

	for (_size_t i = 0; str[i] && i < vidmem_size; i++) {
		putch(str[i]);
	}
}

_size_t get_num_digits(_ssize_t num, _size_t base) {
	_size_t result = 0;

	while (num >= base) {
		result += 1;

		num /= base;
	}

	return MAX(result, 1);
}

const char num_alphabet[] = 
	"0123456789"
	"ABCDEFGHIJ"
	"KLMNOPQRST"
	"UVWXYZ";

const _size_t alphabet_size = sizeof(num_alphabet);

void print_num(_size_t num, _size_t base) {
	if (!vidmem) return;

	if (base <= 0 || base >= alphabet_size) {
		print("Base should be in range 2...");

		print_num(alphabet_size - 1, 10);

		print(", not ");

		print_num(base, 10);

		print("\n\r");

		return;
	}

	if (num < 0) putch('-');

	num = ABS(num);

	if (num < base) {
		putch(num_alphabet[num]);

		return;
	}

	char buf[32] = { 0 };

	_size_t buf_size = sizeof(buf) / sizeof(buf[0]);

	_size_t index = MIN(buf_size - 1, get_num_digits(num, base));

	while (num >= base && index > 0) {
		buf[index] = num_alphabet[num % base];

		num /= base;

		index -= 1;
	}
	
	buf[index] = num_alphabet[num % base];

	print(buf);
}

void nblk__set_cursor_pos(_ssize_t x, _ssize_t y) {
	cur_x = x; cur_y = y;
}

#define set_cursor_pos nblk__set_cursor_pos

void get_cursor_pos(_ssize_t* x, _ssize_t* y) {
	if (x) *x = cur_x;

	if (y) *y = cur_y;
}

void set_style(byte style) {
	cur_style = style;
}

byte get_style() {
	return cur_style;
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

bool cmos_update_in_progress() {
	out8(0x70, 0xA);

	byte reg_a = in8(0x71);

	return (reg_a & CMOS_REGISTER_A_UPDATE_IN_PROGRESS) != 0;
}

void show_rtc_time() {
	out8(0x70, 0xB);

	byte reg_b = in8(0x71);
	
	out8(0x70, 0);

	byte second = from_bcd(in8(0x71));

	out8(0x70, 2);

	byte minute = from_bcd(in8(0x71));

	out8(0x70, 4);

	byte hour = in8(0x71);

	bool pm = false;

	if ((reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {
		pm = hour & 0x80;

		hour &= 0x7F;
	}

	hour = from_bcd(hour);

	print_num(hour, 10); putch(':');

	print_num(minute, 10); putch(':');

	print_num(second, 10);

	if ((reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {
		if (pm) print(" PM");

		else print(" AM");
	}
}

byte read_rtc_seconds() {
	out8(0x70, 0x0B);

	byte reg_b = in8(0x71);

	out8(0x70, 0x00);

	byte second = in8(0x71);

	if ((reg_b & CMOS_REGISTER_B_IS_BINARY_MODE) == 0) {
		second = from_bcd(second);
	}

	return second;
}

void kmain(uint32 magic, void* ptr) {
	vidmem = (uint16*)getMemoryOffset(PMT_TEXT_MEM_80x25);

	byte style = COLOR_BRIGHT_WHITE | (COLOR_BLACK << 4);

	clear_screen(style);

	set_style(style);

	out8(0x70, 0x0B);

	print("Register B: "); print_num(in8(0x71), 2); print("\n\r");

	print("Second: "); print_num(read_rtc_seconds(), 10); print("\n\r");

	print("TSC Calibrating in progress...\n\r");

	uint64 tsc_calibrate_time = 0;

	#define CALIBRATE_SECONDS 5
	
	_time_t cur_calibrate_second = 0;

	byte st_seconds = read_rtc_seconds();

	while (read_rtc_seconds() == st_seconds);

	while (cur_calibrate_second < CALIBRATE_SECONDS) {
		uint64 start = read_tsc();

		st_seconds = read_rtc_seconds();

		while (read_rtc_seconds() == st_seconds);

		// print("                         \r");

		uint64 dur = read_tsc() - start;

		tsc_calibrate_time += dur;

		print("Calculated time: "); print_num(dur, 10); putch(' '); 
		print_num(cur_calibrate_second, 10); putch('/'); print_num(CALIBRATE_SECONDS, 10);
		print("\n\r");

		cur_calibrate_second += 1;
	}

	uint64 avg_tsc_s = tsc_calibrate_time / CALIBRATE_SECONDS;

	print("TSC Calibration done!\n\r");

	print("Result: "); print_num(avg_tsc_s, 10); print(" ticks/s\n\r");

	for (_size_t i = 0; i < 16; i++) {
		set_style(i);

		print("Hello, world!\n\r");
	}

	reboot();
}