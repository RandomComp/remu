#include "main.h"

// Код этого эмулятора 0xEA32

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

#include "math/math.h"

#include "kernel.h"

#include "utils.h"

#include "emulator.h"

#ifdef IS_UNIX
struct termios* orig_termios;
#endif

FILE* log_file = nullptr;

emulator_t* emulator;

// TODO: Сделать двойной буфер видеопамяти

// TODO: Сделать таблицу прерываний и их настройку

static void on_emulator_exit() {
	switch_to_default(orig_termios);

	free(orig_termios);
}

static log_severity_e min_log_severity = LOG_SEVERITY_VERBOSE; // 0 для отключения логов

static log_severity_e max_log_severity = 10; // 0 для отключения логов

static log_severity_e min_log_severity_showing = 0; // 0 для отключения логов

static log_severity_e max_log_severity_showing = 0; // 0 для отключения логов

void emulator_log(bool mirror_stdout, log_severity_e severity, const char* format, ...) {
	if (severity >= max_log_severity || 
		severity < min_log_severity) return;

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
		[LOG_SEVERITY_VERBOSE] 	=	"VERB",
		[LOG_SEVERITY_INFO] 	=	"INFO",
		[LOG_SEVERITY_WARNING] 	= 	"WARN",
		[LOG_SEVERITY_ERROR] 	= 	"ERROR"
	};

	const c_str severities_color[] = {
		[LOG_SEVERITY_VERBOSE] 	=	white_fg,
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

	if ((!log_file || mirror_stdout) && 
		severity >= min_log_severity_showing && 
		severity < max_log_severity_showing) {
		printf(default_style "[%s][%s]%s ", str_buf_time, severities_name[severity_index], severities_color[severity_index]);

		vprintf(format, args2);

		printf(default_style);

		putchar('\r');
	}
}

void exit_emulator(int code) {
	uint64 start_cpu_tsc = emulator && emulator->cpu ? emulator->cpu->tsc_start : emulator_read_tsc();

	uint64 working_cpu_tsc = emulator_read_tsc() - start_cpu_tsc;

	struct timespec ts;

	timespec_get(&ts, TIME_UTC);

	uint64 emulator_working_time = ((ts.tv_nsec / 1000000) + (ts.tv_sec * 1000)) - emulator->emulator_start_time;

	emulator_log(true, LOG_SEVERITY_INFO, "Uptime: %llu ms\n", emulator_working_time);
	
	emulator_log(true, LOG_SEVERITY_INFO, "Timer ticks: %li (1 tick is ~1 ms)\n", emulator->ticks);
	
	emulator_log(true, LOG_SEVERITY_INFO, "CPU TSC since starting: %llu\n", working_cpu_tsc);

	free_emulator(emulator); emulator = nullptr;

	printf(visible_cursor);

	if (log_file) fclose(log_file);
	
	log_file = nullptr;

	exit(code);
}

static void timer_handler(int sig) {
	emulator_update_all(emulator);
}

// extern void printf(const char* format, int len);

// extern void exit123(int);

int main() {
	#ifdef IS_WIN
	printf("This emulator now is not fully supported on windows, use it for your own risk");
	
	return 1;
	#endif

	struct timespec ts;

	timespec_get(&ts, TIME_UTC);

	log_file = fopen("emulator.log", "a");

	if (!log_file) {
		printf("Unable to open log file, log will be shadowing in stdout\n\r");
	}

	time_t cur_time = time(0);

	struct tm* local_time = localtime(&cur_time);

	char buf[32];

	strftime(buf, 32, "%d %B %Y", local_time);

	emulator_log(true, LOG_SEVERITY_INFO, EMULATOR_INFO_FULL_STR "\n");
	emulator_log(true, LOG_SEVERITY_INFO, "Project github: https://github.com/RandomComp/Emulator_OS\n");
	emulator_log(true, LOG_SEVERITY_INFO, "Running on %s\n\r", buf);

	change_title("OS Emulator (Ctrl+C to exit)");

	#ifdef IS_UNIX
	orig_termios = switch_to_raw();
	#endif

	atexit(on_emulator_exit);

	#ifdef IS_UNIX
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
	#endif

	setbuf(stdout, NULL);
	
	ssize_t columns = 80, rows = 25;

	// get_terminal_size(&columns, &rows);
	
	timespec_get(&ts, TIME_UTC);

	uint64 init_start_time = ts.tv_nsec / 1000;

	emulator = init_emulator(columns, rows, FRAMETIME_NS, HALTED_FRAMETIME_NS);

	printf(invisible_cursor);

	timespec_get(&ts, TIME_UTC);

	uint64 emulator_init_dur = (ts.tv_nsec / 1000) - init_start_time;

	emulator_log(true, LOG_SEVERITY_INFO, "Initialization duration: %llu us\n\r", emulator_init_dur);

	run_emulator(emulator, kmain);

	free_emulator(emulator);

	printf(visible_cursor);

	if (log_file) fclose(log_file);
	
	log_file = nullptr;

	return 0;
}