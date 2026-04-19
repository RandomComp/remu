#include "drivers/time/tsc.h"

#include "types.h"

#ifndef FREE_STANDING_MODE
#include "main.h"

#include "cpu/emulator_cpu.h"

extern cpu_t* cpu;
#endif

uint64 read_tsc() {
	uint32 lo = 0, hi = 0;
	
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

	uint64 result = ((uint64)hi << 32) | ((uint64)lo);

	#ifndef FREE_STANDING_MODE
	if (cpu) result -= cpu->tsc_start;
	#endif

	return result;
}