#ifndef _EMULATOR_CPU_H
#define _EMULATOR_CPU_H

#include "types.h"

#include <time.h>

typedef struct cpu_t {
	bool halted;

	timer_t hardware_timerid;

	time_t frametime_ns;

	time_t halted_frametime_ns;

	time_t tsc_start;
} cpu_t;

cpu_t* init_cpu(void (*tick)(int));

void set_halt(cpu_t* cpu);

void clear_halt(cpu_t* cpu);

uint64 get_itval_ns(cpu_t* cpu);

void free_cpu(cpu_t* cpu);

void setup_signals();

#endif