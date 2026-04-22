#ifndef _EMULATOR_TIME_H
#define _EMULATOR_TIME_H

#include "types.h"

typedef struct struct_time_t {
	_time_t second;
	_time_t minute;
	_time_t hour;
	_time_t day_of_week;
	_time_t day_of_month;
	_time_t month;
	_time_t year;
} struct_time_t;

static inline _time_t second_from_unix_time(_time_t time) {
	return time % 60;
}

static inline _time_t minute_from_unix_time(_time_t time) {
	return (time / 60) % 60;
}

static inline _time_t hour_from_unix_time(_time_t time) {
	return (time / (60 * 60)) % 24;
}

static inline _time_t day_from_unix_time(_time_t time) {
	return time / (60 * 60 * 24);
}

static inline _time_t leap_year_cnt(_time_t year) {
	return (year / 4) - (year / 100) + (year / 400);
}

static inline _time_t is_year_leap(_time_t year) {
	return year % 4 == 0 && year % 100 != 0 || year % 400 == 0;
}

static _time_t year_day_from_unix_time(_time_t time) {
	_time_t days = day_from_unix_time(time);

	_time_t year = 0; _time_t year_days = 365;

	while (days >= year_days) {
		year += 1;

		year_days = is_year_leap(year) ? 366 : 365;

		days -= year_days;
	}

	return days;
}

static _time_t year_from_unix_time(_time_t time) {
	_time_t days = day_from_unix_time(time);

	_time_t year = 0; _time_t year_days = 365;

	while (days >= year_days) {
		year += 1;

		year_days = is_year_leap(year) ? 366 : 365;

		days -= year_days;
	}

	return 1970 + year;
}

static inline _time_t century_from_unix_time(_time_t time) {
	_time_t year = year_from_unix_time(time);

	return (year / 100) % 100;
}

static inline _time_t days_from_year(_time_t year) {
	_time_t coef = leap_year_cnt(year);

	return (year - coef) * 365 + (coef * 366);
}

static inline _time_t minute_to_unix_time(_time_t time) {
	return time * 60;
}

static inline _time_t hour_to_unix_time(_time_t time) {
	return time * (60 * 60);
}

static inline _time_t day_to_unix_time(_time_t time) {
	return time * (60 * 60 * 24);
}

static _time_t year_to_unix_time(_time_t time) {
	return day_to_unix_time(days_from_year(time));
}

static inline _time_t century_to_unix_time(_time_t time) {
	return year_to_unix_time(time * 100);
}

static inline struct_time_t struct_time_from_unix_time(_time_t time) {
	struct_time_t result = { 0 };

	result.second = second_from_unix_time(time);

	result.minute = minute_from_unix_time(time);

	result.hour = hour_from_unix_time(time);

	result.day_of_month = day_from_unix_time(time);

	result.day_of_week = day_from_unix_time(time);

	result.month = day_from_unix_time(time);

	result.year = year_from_unix_time(time);

	return result;
}

static inline _time_t unix_time_from_struct_time(struct_time_t time) {
	_time_t result = time.second;

	result += minute_to_unix_time(time.minute);

	result += hour_to_unix_time(time.hour);

	result += day_to_unix_time(time.day_of_month);

	result += day_to_unix_time(time.month);

	result += year_to_unix_time(time.year);

	return result;
}

#endif