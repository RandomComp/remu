#include "builtins/builtins.h"

#include "types.h"

#ifndef FREE_STANDING_MODE
#ifdef IS_UNIX
#include <unistd.h>
#elif IS_WIN
#include <windows.h>
#endif

#include <time.h>

#include "emulator.h"

extern emulator_t* emulator;

#include "main.h"
#endif

void halt() {
	#ifdef FREE_STANDING_MODE
	asm volatile("hlt");
	#elifdef IS_UNIX
	set_halt(emulator->cpu);
	usleep(emulator->cpu->halted_frametime_ns / (time_t)1000);
	#elifdef IS_WIN
	set_halt(emulator->cpu);
	Sleep(emulator->cpu->halted_frametime_ns / (time_t)1000000);
	#endif
}