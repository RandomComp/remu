#include "types.h"

#include "std.h"

#include "drivers/time/cmos.h"

#include "emush/emush.h"

#include "builtins/builtins.h"

int time_cmd(const byte **argv, size_t argc) {
	while (true) {
		set_cursor_pos(0, 10); show_rtc_time();
		set_cursor_pos(0, 11); show_rtc_date();

		halt();
	}

	return 0;
}
