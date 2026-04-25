#include "drivers/time/tsc.h"

#include "types.h"

#include "std.h"

#include "drivers/time/cmos.h"

#ifndef FREE_STANDING_MODE
#include "main.h"

#include "emulator.h"

extern emulator_t* emulator;
#endif

uint64 read_tsc() {
	uint32 lo = 0, hi = 0;
	
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

	uint64 result = ((uint64)hi << 32) | ((uint64)lo);

	#ifndef FREE_STANDING_MODE
	if (emulator->cpu) result -= emulator->cpu->tsc_start;
	#endif

	return result;
}

uint64 get_tsc_in_s(_time_t seconds) {
	uint64 tsc_calibrate_time = 0;
	
	_time_t cur_calibrate_second = 0;

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
