#ifndef _EMULATOR_CPU_H
#define _EMULATOR_CPU_H

#include "types.h"

#include <time.h>

typedef struct pic_t pic_t;

typedef struct cpu_t {
	pic_t* pic;
	
	bool halted;

	time_t frametime_ns;

	time_t halted_frametime_ns;

	time_t tsc_start;
} cpu_t;

typedef void (*signal_handler_t)(int);

cpu_t* init_cpu(uint64 frametime_ns, uint64 halted_frametime_ns);

void set_halt(cpu_t* cpu);

void clear_halt(cpu_t* cpu);

uint64 cpu_get_itval_ns(cpu_t* cpu);

void free_cpu(cpu_t* cpu);

void release_cpu(cpu_t* cpu);

void reset_cpu(cpu_t* cpu);

void cpu_setup_signals();

#endif
