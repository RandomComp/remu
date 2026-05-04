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

void cli() {
	#ifndef __EMULATOR__
	asm volatile("cli");
	#else
	if (kernel_args.__emulator_cli)
		kernel_args.__emulator_cli();
	#endif
}
void sti() {
	#ifndef __EMULATOR__
	asm volatile("sti");
	#else
	if (kernel_args.__emulator_sti)
		kernel_args.__emulator_sti();
	#endif
}
