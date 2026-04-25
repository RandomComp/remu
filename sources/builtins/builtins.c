#include "builtins/builtins.h"

#include "types.h"

#ifndef FREE_STANDING_MODE
extern void (*__emulator_wait_halt)(void);
#endif

void halt() {
	#ifdef FREE_STANDING_MODE
	asm volatile("hlt");
	#else
	__emulator_wait_halt();
	#endif
}
