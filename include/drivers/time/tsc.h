#ifndef _EMULATOR_OS_RDTSC_H
#define _EMULATOR_OS_RDTSC_H

#include "types.h"

uint64 read_tsc();

uint64 get_tsc_in_s(time_t seconds);

#endif
