#include "time/emulator_cmos.h"

#include <stdlib.h>

#include <string.h>

#include <time.h>

#include "emulator.h"

#include "emulator_io.h"

#include "emulator_logger.h"

#include "main.h"

#include "bcd.h"

#include "time/time.h"

static byte attrib_reg = 0;

static cmos_t* cur = nullptr;

static time_t gmtoff = 0;

static void cmos_reg(_size_t _reg) {
	attrib_reg = _reg & 0xFF;
}

static _size_t cmos_get_reg() {
	return attrib_reg;
}

static void cmos_write(_size_t data) {
	if (!cur) return;

	bool is_reg = 	attrib_reg == CMOS_REGISTER_A  || attrib_reg == CMOS_REGISTER_B || 
					attrib_reg == CMOS_REGISTER_C 	|| attrib_reg == CMOS_REGISTER_D;

	if (attrib_reg == CMOS_REGISTER_A) {
		cur->reg_a = data & 0xFF;
	}

	else if (attrib_reg == CMOS_REGISTER_B) {
		cur->reg_b = data & 0xFF;
	}

	else if (attrib_reg == CMOS_REGISTER_C) {
		cur->reg_c = data & 0xFF;
	}

	else if (attrib_reg == CMOS_REGISTER_D) {
		cur->reg_d = data & 0xFF;
	}

	if (is_reg) return;

	struct_time_t rtc_time = struct_time_from_unix_time(cur->unix_time);

	if (attrib_reg == CMOS_RTC_SECONDS) {
		rtc_time.second = data % 60;
	}

	else if (attrib_reg == CMOS_RTC_MINUTES) {
		rtc_time.minute = data % 60;
	}

	else if (attrib_reg == CMOS_RTC_HOURS) {
		rtc_time.hour = (data % 24) - gmtoff;
	}

	else if (attrib_reg == CMOS_RTC_DAY_OF_WEEK) {
		rtc_time.day_of_week = data % 7;
	}

	else if (attrib_reg == CMOS_RTC_DAY_OF_MONTH) {
		rtc_time.month = data % 12;
	}

	else if (attrib_reg == CMOS_RTC_YEARS) {
		rtc_time.year = data % 100;
	}

	else if (attrib_reg == CMOS_RTC_CENTURY) {
		rtc_time.year += data * 100;
	}

	cur->unix_time = unix_time_from_struct_time(rtc_time);
}

static _size_t cmos_read() {
	if (!cur) return 0;

	_ssize_t reg_val = -1;

	if (attrib_reg == CMOS_REGISTER_A) {
		reg_val = (_ssize_t)cur->reg_a;
	}

	else if (attrib_reg == CMOS_REGISTER_B) {
		reg_val = (_ssize_t)cur->reg_b;
	}

	else if (attrib_reg == CMOS_REGISTER_C) {
		cur->reg_c = 0;

		reg_val = (_ssize_t)cur->reg_c;
	}

	else if (attrib_reg == CMOS_REGISTER_D) {
		reg_val = (_ssize_t)cur->reg_d;
	}

	if (reg_val >= 0) {
		return reg_val;
	}

	_size_t result = 0;

	struct_time_t struct_time = struct_time_from_unix_time(cur->unix_time);

	if (attrib_reg == CMOS_RTC_SECONDS) {
		result = struct_time.second;
	}

	else if (attrib_reg == CMOS_RTC_MINUTES) {
		result = struct_time.minute;
	}

	else if (attrib_reg == CMOS_RTC_HOURS) {
		result = struct_time.hour;

		byte pm = result >= 12;

		if ((cur->reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {
			result %= 12;
		}
		
		result = ((cur->reg_b & 0x4) != 0) ? result : to_bcd(result);

		if ((cur->reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {
			result |= pm << 7;
		}

		return result;
	}

	else if (attrib_reg == CMOS_RTC_DAY_OF_WEEK) {
		result = struct_time.day_of_week;
	}

	else if (attrib_reg == CMOS_RTC_DAY_OF_MONTH) {
		result = struct_time.day_of_month;
	}

	else if (attrib_reg == CMOS_RTC_MONTHS) {
		result = struct_time.month;
	}

	else if (attrib_reg == CMOS_RTC_YEARS) {
		result = struct_time.year % 100;
	}

	else if (attrib_reg == CMOS_RTC_CENTURY) {
		result = struct_time.year / 100;
	}

	result = ((cur->reg_b & 0x4) != 0) ? result : to_bcd(result);

	return result;
}

time_t get_gmtoff() {
	time_t _time = time(0);

	struct tm* loc = localtime(&_time);

	return loc->tm_gmtoff;
}

static void update_cmos() {
	if (!cur) return;

	cur->reg_a |= 0x80;

	cur->unix_time++;

	cur->reg_a &= ~0x80;
}

cmos_t* init_cmos() {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "CMOS initialization...");

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
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up timer (1 hz) for CMOS updating...");

	emulator_setup_tick_timer(nullptr, update_cmos, 1000);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up ports (0x70, 0x71) for CMOS...");

	emulator_setup_port_out(0x70, cmos_reg);
	emulator_setup_port_in(0x70, cmos_get_reg);

	emulator_setup_port_out(0x71, cmos_write);
	emulator_setup_port_in(0x71, cmos_read);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "CMOS initialized");

	return cmos;
}

void free_cmos(cmos_t* cmos) {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "CMOS deinitialization...");

	if (cmos) free(cmos);

	if (cur == cmos) {
		cur = nullptr;
	}

	emulator_log(false, LOG_SEVERITY_VERBOSE, "CMOS deinitialized");
}

void reset_cmos(cmos_t* cmos) {
	if (!cmos) return;

	emulator_log(false, LOG_SEVERITY_INFO, "CMOS reseting...");

	time_t cur_time = time(0);

	gmtoff = get_gmtoff();

	cmos->unix_time = cur_time + gmtoff;

	cmos->reg_a = 0b010 << 4;

	cmos->reg_b = 0b00000010;

	cmos->reg_c = 0b00000000;

	cmos->reg_d = 0b10000000;

	emulator_log(false, LOG_SEVERITY_INFO, "CMOS reseted");
}

void release_all_cmos() {
	emulator_release_tick_timer(nullptr, update_cmos);

	emulator_release_port_out(0x70);

	emulator_release_port_in(0x70);

	emulator_release_port_out(0x71);

	emulator_release_port_in(0x71);

	if (cur) free(cur);

	cur = nullptr;
}
