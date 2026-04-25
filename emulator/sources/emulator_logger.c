#include "emulator_logger.h"

#include "ansi.h"

#include "emulator.h"

#include "math/math.h"

#include <stdio.h>

static log_severity_e min_log_severity = LOG_SEVERITY_VERBOSE; // 0 для отключения логов

static log_severity_e max_log_severity = 10; // 0 для отключения логов

static log_severity_e min_log_severity_showing = 0; // 0 для отключения логов

static log_severity_e max_log_severity_showing = 10; // 0 для отключения логов

static char* msg = nullptr; static size_t msg_size = 0;

static char* last_msg = nullptr; static size_t last_msg_size = 0, repeated_cnt = 0;

static FILE* log_file = nullptr; static long repeated_log_start = 0;

static emulator_t* emulator = nullptr;

void init_emulator_logger() {
	log_file = fopen("emulator.log", "a");

	if (!log_file) {
		printf("Unable to open log file, log will be shadowing in stdout\n\r");
	}
}

void emulator_logger_set_emulator(emulator_t* _emulator) {
	if (!_emulator) return;

	emulator = _emulator;
}

void emulator_log(bool mirror_stdout, log_severity_e severity, const char* format, ...) {
	if (severity >= max_log_severity || 
		severity < min_log_severity) return;

	#ifdef EMULATOR_SDL_USING
	mirror_stdout = true;

	min_log_severity_showing = 0;

	max_log_severity_showing = 10;
	#endif

	va_list args, args2;

	va_start(args, format);

	va_copy(args2, args);

	const c_str severities_name[] = {
		[LOG_SEVERITY_VERBOSE] 	=	"VERB",
		[LOG_SEVERITY_INFO] 	=	"INFO",
		[LOG_SEVERITY_WARNING] 	= 	"WARN",
		[LOG_SEVERITY_ERROR] 	= 	"ERROR"
	};

	const c_str severities_color[] = {
		[LOG_SEVERITY_VERBOSE] 	=	white_fg,
		[LOG_SEVERITY_INFO] 	= 	bold white_fg,
		[LOG_SEVERITY_WARNING] 	= 	bold bright_yellow_fg,
		[LOG_SEVERITY_ERROR] 	= 	bold red_fg	
	};

	const _size_t severities_cnt = sizeof(severities_name) / sizeof(severities_name[0]);

	_size_t severity_index = MIN(severities_cnt, severity);

	uint64 ticks = emulator ? emulator->ticks : 0;

	size_t msg_len = vsnprintf(null, 0, format, args2) + 1;

	if (!msg || msg_size < msg_len) {
		msg = realloc(msg, msg_len);

		msg_size = msg_len;
	}

	memset(msg, 0, msg_len);

	vsnprintf(msg, msg_len, format, args);

	if (last_msg && strcmp(last_msg, msg) == 0)
		repeated_cnt++;

	else {
		if (last_msg && repeated_cnt > 0) {
			fprintf(log_file, "... and repeated %zu times\n", repeated_cnt);
		}

		repeated_cnt = 0;
	}

	if (log_file) {
		if (repeated_cnt == 0) {
			fprintf(log_file, "[%llu][%s] %s\n", ticks, severities_name[severity_index], msg);
		}

		if (severity >= LOG_SEVERITY_WARNING) fflush(log_file);
	}

	if ((!log_file || mirror_stdout) && 
		severity >= min_log_severity_showing && 
		severity < max_log_severity_showing) {
		if (repeated_cnt == 0) {
			printf(default_style "\n\r[%llu][%s]%s %s" default_style, ticks, severities_name[severity_index], severities_color[severity_index], msg);
		}
		
		else {
			if (repeated_cnt == 1) printf("\n\r");

			printf("\r" default_style "... and repeated %zu times" default_style, repeated_cnt);
		}

		// fflush(stdout);

		if (severity >= LOG_SEVERITY_WARNING) fflush(stdout);
	}

	if (!last_msg || last_msg_size < msg_len) {
		last_msg = realloc(last_msg, msg_len);

		last_msg_size = msg_len;
	}

	memcpy(last_msg, msg, msg_len);
}

void free_emulator_logger() {
	if (msg) free(msg);

	msg = nullptr; msg_size = 0;

	if (last_msg) free(last_msg);

	last_msg = nullptr; last_msg_size = 0;

	printf("\n\r");
}
