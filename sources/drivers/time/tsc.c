#include "drivers/time/tsc.h"

#include "types.h"

#include "std.h"

#include "drivers/time/cmos.h"

uint64 read_tsc() {
	uint32 lo = 0, hi = 0;
	
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

	uint64 result = ((uint64)hi << 32) | ((uint64)lo);

	return result;
}

uint64 get_tsc_in_s(time_t seconds) {
	uint64 tsc_calibrate_time = 0;
	
	time_t cur_calibrate_second = 0;

	byte st_seconds = read_rtc_seconds();

	while (read_rtc_seconds() == st_seconds);

	while (cur_calibrate_second < seconds) {
		uint64 start = read_tsc();

		st_seconds = read_rtc_seconds();

		while (read_rtc_seconds() == st_seconds);

		uint64 dur = read_tsc() - start;

		tsc_calibrate_time += dur;

		cur_calibrate_second += 1;
	}

	uint64 avg_tsc_s = tsc_calibrate_time / seconds;

	return avg_tsc_s;
}
