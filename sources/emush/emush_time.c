#include "types.h"

#include "std/stdio.h"

#include "drivers/time/cmos.h"

#include "emush/emush.h"

#include "builtins/builtins.h"

int time_cmd(const byte **argv, size_t argc) {
	show_rtc_time(); kprint("\n\r");
	show_rtc_date(); kprint("\n\r");

	return 0;
}
