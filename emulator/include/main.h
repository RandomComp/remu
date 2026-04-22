#ifndef _EMULATOR_MAIN_H
#define _EMULATOR_MAIN_H

#include "types.h"

typedef enum log_severity_e {
	LOG_SEVERITY_VERBOSE,
	LOG_SEVERITY_INFO,
	LOG_SEVERITY_WARNING,
	LOG_SEVERITY_ERROR,
} log_severity_e;

void emulator_log(bool mirror_stdout, log_severity_e severity, const char* format, ...);

#endif