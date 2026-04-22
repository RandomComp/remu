#ifndef _EMULATOR_H
#define _EMULATOR_H

#include "cpu/emulator_cpu.h"

#include "memory/emulator_ram.h"

#include "vga/emulator_vga.h"

#include "time/emulator_cmos.h"

#include "hid/emulator_kbdps2.h"

#include "power/emulator_power_control.h"

#include "emulator_multiboot.h"

#include "emulator_io.h"

#include "types.h"

#define FRAMETIME_NS (1000 * 1000 * 20)

#define HALTED_FRAMETIME_NS (1000 * 1000 * 200)

#define TICK_TIMERS_SIZE_STEP (4)

typedef void (*tick_timer_handler_t)();

typedef struct tick_timer_t {
	_time_t last_time;

	_time_t ms;

	tick_timer_handler_t handler;
} tick_timer_t;

typedef struct emulator_t {
	cpu_t* cpu;

	ram_t* ram;

	vga_text_screen_t* vga;

	cmos_t* cmos;

	kbdps2_t* kbdps2;

	tick_timer_t* tick_timers;

	size_t tick_timers_cnt; 
	
	size_t tick_timers_size;

	uint64 ticks;

	uint64 emulator_start_time;

	bool is_hardware_reseting;
} emulator_t;

#define EMULATOR_VERSION_STR "beta 0.0.5"

#define EMULATOR_VERSION_FULL_STR EMULATOR_VERSION_STR " (" __DATE__ ", " __TIME__ ") for " PLATFORM_NAME " using " PLATFORM_COMPILER_NAME " " PLATFORM_ARCH

#define EMULATOR_INFO_FULL_STR "OS Emulator (GPL V3.0) " EMULATOR_VERSION_FULL_STR " by RDev."

#define EMULATOR_INFO_STR "OS Emulator " EMULATOR_VERSION_STR " by RDev"

void emulator_setup_tick_timer(emulator_t* emulator, tick_timer_handler_t handler, _time_t ms);

void emulator_release_tick_timer(emulator_t* emulator, tick_timer_handler_t handler);

void reset_emulator(emulator_t* emulator, int code);

void run_emulator(emulator_t* emulator, void (*kmain)(uint32 magic, multiboot_info_t* multiboot));

void emulator_forced_update_all_timers(emulator_t* emulator);

void emulator_update_all(emulator_t* emulator);

emulator_t* init_emulator(_ssize_t columns, _ssize_t rows, uint64 frametime_ns, uint64 halted_frametime_ns);

void free_emulator(emulator_t* emulator);

#endif