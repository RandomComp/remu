#include "time/emulator_cmos.h"

#include <stdlib.h>

#include <string.h>

#include <time.h>

#include "main.h"

#include "builtins/bcd.h"

#include "time/time.h"

static byte reg = 0;

static cmos_t* cur = nullptr;

static time_t gmtoff = 0;

static void cmos_reg(_size_t _reg) {
	reg = _reg & 0xFF;
}

static _size_t cmos_get_reg() {
	return reg;
}

static void cmos_write(_size_t data) {
	if (!cur) return;

	bool is_reg = 	reg == CMOS_REGISTER_A  || reg == CMOS_REGISTER_B || 
					reg == CMOS_REGISTER_C 	|| reg == CMOS_REGISTER_D;

	if (reg == CMOS_REGISTER_A) {
		cur->reg_a = data & 0xFF;
	}

	else if (reg == CMOS_REGISTER_B) {
		cur->reg_b = data & 0xFF;
	}

	else if (reg == CMOS_REGISTER_C) {
		cur->reg_c = data & 0xFF;
	}

	else if (reg == CMOS_REGISTER_D) {
		cur->reg_d = data & 0xFF;
	}

	if (is_reg) return;

	struct_time_t rtc_time = struct_time_from_unix_time(cur->unix_time);

	if (reg == CMOS_RTC_SECONDS) {
		rtc_time.second = data % 60;
	}

	else if (reg == CMOS_RTC_MINUTES) {
		rtc_time.minute = data % 60;
	}

	else if (reg == CMOS_RTC_HOURS) {
		rtc_time.hour = (data % 24) - gmtoff;
	}

	else if (reg == CMOS_RTC_DAY_OF_WEEK) {
		rtc_time.day_of_week = data % 7;
	}

	else if (reg == CMOS_RTC_DAY_OF_MONTH) {
		rtc_time.month = data % 12;
	}

	else if (reg == CMOS_RTC_YEARS) {
		rtc_time.year = data % 100;
	}

	else if (reg == CMOS_RTC_CENTURY) {
		rtc_time.year += data * 100;
	}

	cur->unix_time = unix_time_from_struct_time(rtc_time);
	
	reg = 0;
}

static _size_t cmos_read() {
	if (!cur) return 0;

	_ssize_t reg_val = -1;

	if (reg == CMOS_REGISTER_A) {
		reg_val = (_ssize_t)cur->reg_a;
	}

	else if (reg == CMOS_REGISTER_B) {
		reg_val = (_ssize_t)cur->reg_b;
	}

	else if (reg == CMOS_REGISTER_C) {
		cur->reg_c = 0;

		reg_val = (_ssize_t)cur->reg_c;
	}

	else if (reg == CMOS_REGISTER_D) {
		reg_val = (_ssize_t)cur->reg_d;
	}

	if (reg_val >= 0) {
		reg = 0;

		return reg_val;
	}

	_size_t result = 0;

	if (reg == CMOS_RTC_SECONDS) {
		result = (_size_t)second_from_unix_time(cur->unix_time);
	}

	else if (reg == CMOS_RTC_MINUTES) {
		result = (_size_t)minute_from_unix_time(cur->unix_time);
	}

	else if (reg == CMOS_RTC_HOURS) {
		result = (_size_t)hour_from_unix_time(cur->unix_time);

		if ((cur->reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {
			byte pm = result >= 12;

			result %= 12;

			result |= pm << 7;
		}
	}

	else if (reg == CMOS_RTC_DAY_OF_WEEK) {
		result = (_size_t)day_from_unix_time(cur->unix_time);
	}

	else if (reg == CMOS_RTC_DAY_OF_MONTH) {
		result = (_size_t)day_from_unix_time(cur->unix_time);
	}

	else if (reg == CMOS_RTC_YEARS) {
		result = (_size_t)year_from_unix_time(cur->unix_time) % 100;
	}

	else if (reg == CMOS_RTC_CENTURY) {
		result = (_size_t)century_from_unix_time(cur->unix_time);
	}

	result = ((cur->reg_b & 0x4) != 0) ? result : to_bcd(result);

	reg = 0;

	return result;
}

time_t get_gmtoff() {
	time_t _time = time(0);

	struct tm* loc = localtime(&_time);

	return loc->tm_gmtoff;
}

void update_cmos() {
	if (!cur) return 0;

	cur->reg_a |= 0x80;

	cur->unix_time++;

	cur->reg_a &= ~0x80;
}

cmos_t* init_cmos() {
	cmos_t* cmos = malloc(sizeof(cmos_t));

	memset(cmos, 0, sizeof(cmos_t));

	time_t cur_time = time(0);

	gmtoff = get_gmtoff();

	cmos->unix_time = cur_time + gmtoff;

	cmos->reg_a = 0b010 << 4;

	cmos->reg_b = 0b00000010;

	cmos->reg_c = 0b00000000;

	cmos->reg_d = 0b10000000;

	cur = cmos;

	setup_tick_timer(update_cmos, 1000);

	setup_port_out(0x70, cmos_reg);

	setup_port_in(0x70, cmos_get_reg);

	setup_port_out(0x71, cmos_write);

	setup_port_in(0x71, cmos_read);

	return cmos;
}

void free_cmos(cmos_t* cmos) {
	if (!cmos) return;

	free(cmos);
}