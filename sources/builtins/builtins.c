#include "builtins/builtins.h"

#include "types.h"

#ifndef FREE_STANDING_MODE
#include "kernel.h"

extern __init_kernel_args_t kernel_args;
#endif

void halt() {
	#ifdef FREE_STANDING_MODE
	asm volatile("hlt");
	#else
	if (kernel_args.__emulator_wait_halt)
		kernel_args.__emulator_wait_halt();
	#endif
}
