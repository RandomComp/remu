#include "types.h"

#include "std.h"

#include "drivers/time/cmos.h"

#include "emush/emush.h"

int time_cmd(const byte **argv, size_t argc) {
	show_rtc_time(); kprint("\n");
	show_rtc_date(); kprint("\n");

	return 0;
}
