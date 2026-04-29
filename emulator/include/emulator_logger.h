#ifndef _EMULATOR_LOGGER_H
#define _EMULATOR_LOGGER_H

#include "types.h"

#include "emulator_fwd.h"

#include <stddef.h>

#include <bits/types/FILE.h>

typedef enum log_severity_e {
	LOG_SEVERITY_TRACE,
	LOG_SEVERITY_VERBOSE,
	LOG_SEVERITY_INFO,
	LOG_SEVERITY_WARNING,
	LOG_SEVERITY_ERROR,
	LOG_SEVERITY_REPORT,
} log_severity_e;

typedef struct logger_t {
	char* msg; 		size_t msg_size;

	char* last_msg; size_t last_msg_size, repeated_cnt;

	FILE* log_file;
} logger_t;

logger_t* init_emulator_logger();

void emulator_logger_set_emulator(emulator_t* _emulator);

void emulator_log(bool mirror_stdout, log_severity_e severity, const char* format, ...);

void free_emulator_logger(logger_t* logger);

#endif
