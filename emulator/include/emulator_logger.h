#ifndef _EMULATOR_LOGGER_H
#define _EMULATOR_LOGGER_H

#include "types.h"

#include "emulator_fwd.h"

typedef enum log_severity_e {
	LOG_SEVERITY_VERBOSE,
	LOG_SEVERITY_INFO,
	LOG_SEVERITY_WARNING,
	LOG_SEVERITY_ERROR,
} log_severity_e;

void init_emulator_logger();

void emulator_logger_set_emulator(emulator_t* _emulator);

void emulator_log(bool mirror_stdout, log_severity_e severity, const char* format, ...);

void free_emulator_logger();

#endif
