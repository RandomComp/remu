#include "builtins/builtins.h"

#include "types.h"

#ifdef __EMULATOR__
#include "kernel.h"

extern __init_kernel_args_t kernel_args;
#endif

void halt() {
	#ifndef __EMULATOR__
	asm volatile("hlt");
	#else
	if (kernel_args.__emulator_wait_halt)
		kernel_args.__emulator_wait_halt();
	#endif
}
