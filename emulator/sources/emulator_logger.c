#include "emulator_logger.h"

#include "ansi.h"

#include "emulator.h"

#include "math/math.h"

#include "utils.h"

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <stdarg.h>

#if defined(IS_UNIX)
#include <unistd.h>
#elif defined(IS_WIN)
#include <unistd.h>
#endif

static log_severity_e min_log_severity = LOG_SEVERITY_VERBOSE;
static log_severity_e max_log_severity = 10; // 0 для отключения логов

static log_severity_e min_log_severity_showing = 0;
static log_severity_e max_log_severity_showing = 10; // 0 для отключения логов

static logger_t* cur = nullptr;

logger_t* init_emulator_logger() {
	logger_t* logger = malloc(sizeof(logger_t));

	memset(logger, 0, sizeof(logger_t));

	logger->start_tsc = emulator_read_tsc();

	logger->log_file = fopen("emulator.log", "a");

	if (!logger->log_file) {
		printf("Unable to open log file, log will be shadowing in stdout\n\r");
	}

	cur = logger;

	return logger;
}

static uint64 tsc_in_s = 0;

static bool first = true;

// Log any emulator message to file and terminal, 
void emulator_log(bool mirror_stdout, log_severity_e severity, const char* format, ...) {
	if (severity >= max_log_severity || 
		severity < min_log_severity) return;
	
	if (!cur) return;

	if (first) {
		uint64 start = emulator_read_tsc();

		#if defined(IS_UNIX)
		usleep(100 * 1000);
		#elif defined(IS_WIN)
		Sleep(100);
		#endif

		tsc_in_s = (emulator_read_tsc() - start) * 10;

		first = false;
	}

	float ticks = (float)(emulator_read_tsc()) - (float)(cur->start_tsc);

	if (tsc_in_s > 0) {
		ticks /= (float)(tsc_in_s);
	}

	#ifdef EMULATOR_SDL_USING
	mirror_stdout = true;

	min_log_severity_showing = 0;

	max_log_severity_showing = 10;
	#endif

	va_list args, args2;

	va_start(args, format);

	va_copy(args2, args);

	const c_str severities_name[] = {
		[LOG_SEVERITY_TRACE] 	=	"TRCE",
		[LOG_SEVERITY_VERBOSE] 	=	"VERB",
		[LOG_SEVERITY_INFO] 	=	"INFO",
		[LOG_SEVERITY_WARNING] 	= 	"WARN",
		[LOG_SEVERITY_ERROR] 	= 	"ERROR",
		[LOG_SEVERITY_REPORT] 	= 	"KERNEL"
	};

	const c_str severities_color[] = {
		[LOG_SEVERITY_TRACE] 	=	white_fg,
		[LOG_SEVERITY_VERBOSE] 	=	white_fg,
		[LOG_SEVERITY_INFO] 	= 	bold white_fg,
		[LOG_SEVERITY_WARNING] 	= 	bold bright_yellow_fg,
		[LOG_SEVERITY_ERROR] 	= 	bold red_fg,
		[LOG_SEVERITY_REPORT] 	= 	blink bold red_fg	
	};

	const _size_t severities_cnt = sizeof(severities_name) / sizeof(severities_name[0]);

	_size_t severity_index = MIN(severities_cnt, severity);

	size_t msg_len = vsnprintf(null, 0, format, args2) + 1;

	if (!cur->msg || cur->msg_size < msg_len) {
		cur->msg = realloc(cur->msg, msg_len);

		cur->msg_size = msg_len;
	}

	memset(cur->msg, 0, msg_len);

	vsnprintf(cur->msg, msg_len, format, args);

	if (cur->last_msg && strcmp(cur->last_msg, cur->msg) == 0)
		cur->repeated_cnt++;

	else {
		if (cur->last_msg && cur->repeated_cnt > 0) {
			fprintf(cur->log_file, "... and repeated %zu times\n", cur->repeated_cnt);
		}

		cur->repeated_cnt = 0;
	}

	if (cur->log_file) {
		if (cur->repeated_cnt == 0) {
			fprintf(cur->log_file, "[%f][%s] %s\n", ticks, severities_name[severity_index], cur->msg);
		}

		if (severity >= LOG_SEVERITY_WARNING) fflush(cur->log_file);
	}

	if ((!cur->log_file || mirror_stdout) && 
		severity >= min_log_severity_showing && 
		severity < max_log_severity_showing) {
		if (cur->repeated_cnt == 0) {
			printf(default_style "\n\r[%f][%s]%s %s" default_style, ticks, severities_name[severity_index], severities_color[severity_index], cur->msg);
		}
		
		else {
			if (cur->repeated_cnt == 1) printf("\n\r");

			printf("\r" default_style "... and repeated %zu times" default_style, cur->repeated_cnt);
		}

		// fflush(stdout);

		if (severity >= LOG_SEVERITY_WARNING) fflush(stdout);
	}

	if (!cur->last_msg || cur->last_msg_size < msg_len) {
		cur->last_msg = realloc(cur->last_msg, msg_len);

		cur->last_msg_size = msg_len;
	}

	memcpy(cur->last_msg, cur->msg, msg_len);
}

void free_emulator_logger(logger_t* logger) {
	if (!logger) {
		printf("Cannot deinitialize logger: no logger instance provided\n\r");

		return;
	}

	printf("\n\rLogger deinitializing...\n\r");

	printf("Logger msg deinitializing...\n\r");

	if (logger->msg) free(logger->msg);

	logger->msg = nullptr; logger->msg_size = 0;

	printf("Logger msg deinitialized!\n\r");

	printf("Logger last msg deinitializing...\n\r");

	if (logger->last_msg) free(logger->last_msg);

	logger->last_msg = nullptr; logger->last_msg_size = 0;

	printf("Logger last msg deinitialized!\n\r");

	if (logger->log_file)
		free(logger->log_file); logger->log_file = nullptr;

	if (logger) free(logger); logger = nullptr;

	if (cur == logger) cur = nullptr;

	printf("Logger deinitialized!\n\r");
}
