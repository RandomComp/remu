#include "main.h"

#include "types.h"

#include <stdio.h>

#include <stdlib.h>

#ifdef IS_UNIX
#include <unistd.h>

#include <fcntl.h>

#include <sys/time.h>
#endif

#include <time.h>

#include <string.h>

#include "ansi.h"

#include "kernel.h"

#include "math/math.h"

#include "drivers/time/tsc.h"

#include "cpu/emulator_cpu.h"

#include "vga/emulator_vga.h"

#include "time/emulator_cmos.h"

#include "hid/emulator_kbdps2.h"

#include "power/emulator_power_control.h"

#include "utils.h"

#define frametime_ns 1000 * 1000 * 20

#define halted_frametime_ns 1000 * 1000 * 200

#define EMULATOR_VERSION_STR "beta 0.0.5 (" __DATE__ ", " __TIME__ ") for " PLATFORM_NAME " using " PLATFORM_COMPILER_NAME " " PLATFORM_ARCH

cpu_t* cpu = nullptr;

vga_text_screen_t* vga = nullptr;

cmos_t* cmos = nullptr;

kbdps2_t* kbdps2 = nullptr;

struct termios* orig_termios;

// TODO: Сделать двойной буфер видеопамяти

// TODO: Сделать таблицу прерываний и их настройку

static handler_port_in_t table_port_in[65536] = { 0 };

static handler_port_out_t table_port_out[65536] = { 0 };

#define TICK_TIMERS_SIZE_STEP 4

static tick_timer_t* tick_timers = nullptr;
static size_t tick_timers_cnt = 0; static size_t tick_timers_size = 0;

static time_t ticks = 0;

FILE* log_file = nullptr;

uint64 emulator_start_time = 0;

static void update_all();

_size_t port_in(uint16 port) {
	handler_port_in_t result = table_port_in[port];

	if (result)
		return result();

	return 0;
}

void port_out(uint16 port, _size_t value) {
	handler_port_out_t result = table_port_out[port];

	if (result)
		result(value);
}

void setup_port_in(uint16 port, handler_port_in_t handler) {
	table_port_in[port] = handler;
}

void setup_port_out(uint16 port, handler_port_out_t handler) {
	table_port_out[port] = handler;
}

void setup_tick_timer(tick_timer_handler_t handler, _time_t ms) {
	if (tick_timers_cnt >= tick_timers_size) {
		tick_timers_size += TICK_TIMERS_SIZE_STEP;

		tick_timers = realloc(tick_timers, tick_timers_size * sizeof(tick_timer_t));
 	}

	if (tick_timers) {
		tick_timers[tick_timers_cnt] = (tick_timer_t){ 
			.handler = handler, .ms = ms, .last_time = ticks
		};
	}

	tick_timers_cnt += 1;
}

void release_port_in(uint16 port) {
	table_port_in[port] = nullptr;
}

void release_port_out(uint16 port) {
	table_port_out[port] = nullptr;
}

void release_tick_timer(tick_timer_handler_t handler) {
	_ssize_t index = -1;

	for (_size_t i = 0; i < tick_timers_cnt; i++) {
		if (tick_timers[i].handler == handler) {
			index = i; break;
		}
	}

	if (index) {
		tick_timers[index] = (tick_timer_t){ 0 };
	}
}

static void forced_update_all_timers() {
	if (tick_timers) {
		for (size_t i = 0; i < tick_timers_cnt; i++) {
			tick_timer_t* tick_timer = &tick_timers[i];

			if (!tick_timer) continue;

			tick_timer->handler();
		}
	}
}

static void update_all() {
	if (tick_timers) {
		for (size_t i = 0; i < tick_timers_cnt; i++) {
			tick_timer_t* tick_timer = &tick_timers[i];

			if (!tick_timer) continue;

			_time_t dur = ticks - tick_timer->last_time;

			if (dur >= tick_timer->ms) {
				tick_timer->handler();
				
				tick_timer->last_time = ticks;
			}
		}
	}

	ticks += (time_t)(get_itval_ns(cpu) / 1000000);
}

static void timer_handler(int sig) {
	update_all();
}

void exit_emulator(int code) {
	forced_update_all_timers();

	printf("\n\r");

	uint64 start_cpu_tsc = emulator_read_tsc();
	
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);

	uint64 working_time = ((ts.tv_nsec / 1000000) + (ts.tv_sec * 1000)) - emulator_start_time;

	if (kbdps2) {
		free_kbdps2(kbdps2); kbdps2 = null;

		release_all_kbdps2();
	}

	if (cmos) {
		free_cmos(cmos); cmos = null;

		release_all_cmos();
	}
	
	if (vga) {
		free_vga_text_screen(vga); vga = null;
	}
	
	if (cpu) {
		start_cpu_tsc = cpu->tsc_start;

		clear_halt(cpu);

		release_cpu(cpu); cpu = null;
	}

	release_power_control();

	printf(visible_cursor);

	uint64 working_cpu_tsc = emulator_read_tsc() - start_cpu_tsc;

	emulator_log(true, LOG_SEVERITY_INFO, "Uptime: %llu ms\n\r", working_time);
	
	emulator_log(true, LOG_SEVERITY_INFO, "Timer ticks: %li (1 tick is ~1 ms)\n\r", ticks);
	
	emulator_log(true, LOG_SEVERITY_INFO, "CPU TSC since starting: %llu\n\r", working_cpu_tsc);

	if (log_file) fclose(log_file);
	
	log_file = nullptr;

	// fflush(stdout);

	exit(code);
}

static void on_emulator_exit() {
	switch_to_default(orig_termios);

	free(orig_termios);
}

void emulator_log(bool mirror_stdout, log_severity_e severity, const char* format, ...) {
	va_list args, args2;

	va_start(args, format);

	va_copy(args2, args);

	time_t cur_time = time(0);

	struct tm* local_time = localtime(&cur_time);

	char str_buf_time[32];

	size_t writed_buf_size = strftime(str_buf_time, 32, "%H:%M:", local_time);

	if (writed_buf_size > 0) {
		struct timespec ts;

		timespec_get(&ts, TIME_UTC);

		snprintf(str_buf_time + writed_buf_size, 32 - writed_buf_size, "%02lli.%06lli", ts.tv_sec % 60, (ts.tv_nsec / 1000) % 1000000);
	}

	const c_str severities_name[] = {
		[LOG_SEVERITY_OK] 	=		"OK",
		[LOG_SEVERITY_INFO] 	=	"INFO",
		[LOG_SEVERITY_WARNING] 	= 	"WARN",
		[LOG_SEVERITY_ERROR] 	= 	"ERROR"
	};

	const c_str severities_color[] = {
		[LOG_SEVERITY_OK] 	= 		bold green_fg,
		[LOG_SEVERITY_INFO] 	= 	white_fg,
		[LOG_SEVERITY_WARNING] 	= 	bold bright_yellow_fg,
		[LOG_SEVERITY_ERROR] 	= 	bold red_fg	
	};

	const _size_t severities_cnt = sizeof(severities_name) / sizeof(severities_name[0]);

	_size_t severity_index = MIN(severities_cnt, severity);

	if (log_file) {
		fprintf(log_file, "[%s][%s] ", str_buf_time, severities_name[severity_index]);
		
		vfprintf(log_file, format, args);
	}

	if (!log_file || mirror_stdout) {
		printf(default_style "[%s][%s]%s ", str_buf_time, severities_name[severity_index], severities_color[severity_index]);

		vprintf(format, args2);

		printf(default_style);
	}
}

// extern void printf(const char* format, int len);

// extern void exit123(int);

int main() {
	#ifdef IS_WIN
	printf("This emulator now is unsupported on windows, try again on UNIX-like OS (macOS, Linux or similar)");
	
	exit(1);
	#endif

	struct timespec ts;

	timespec_get(&ts, TIME_UTC);

	emulator_start_time = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);

	log_file = fopen("emulator.log", "a");

	if (!log_file) {
		printf("Unable to open log file, log will be shadowing in stdout\n\r");
	}

	time_t cur_time = time(0);

	struct tm* local_time = localtime(&cur_time);

	char buf[32];

	strftime(buf, 32, "%d %B %Y", local_time);

	emulator_log(true, LOG_SEVERITY_INFO, "OS Emulator (GPL V3.0) version " EMULATOR_VERSION_STR " by RDev.\n");
	emulator_log(true, LOG_SEVERITY_INFO, "Project github: https://github.com/RandomComp/Emulator_OS\n");
	emulator_log(true, LOG_SEVERITY_INFO, "Running on %s\n\r", buf);

	change_title("OS Emulator (Ctrl+C to exit)");

	timespec_get(&ts, TIME_UTC);

	uint64 init_start_time = ts.tv_nsec / 1000;

	orig_termios = switch_to_raw();

	atexit(on_emulator_exit);

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	setbuf(stdout, NULL);
	
	ssize_t columns = 80, rows = 25;

	// get_terminal_size(&columns, &rows);

	cpu = init_cpu(frametime_ns, halted_frametime_ns, &timer_handler);

	vga = init_vga_text_screen(columns, rows);

	cmos = init_cmos();

	kbdps2 = init_kbdps2();

	init_power_control();

	printf(invisible_cursor);

	timespec_get(&ts, TIME_UTC);

	uint64 emulator_init_dur = (ts.tv_nsec / 1000) - init_start_time;

	emulator_log(true, LOG_SEVERITY_INFO, "Initialization duration: %llu us\n\r", emulator_init_dur);

	kmain(0x2BADB002, nullptr);

	exit_emulator(0);

	return 0;
}