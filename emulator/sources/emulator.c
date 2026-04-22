#include "emulator.h"

#include "cpu/emulator_cpu.h"

#include "memory/emulator_ram.h"

#include "vga/emulator_vga.h"

#include "time/emulator_cmos.h"

#include "hid/emulator_kbdps2.h"

#include "power/emulator_power_control.h"

#include "emulator_multiboot.h"

#include "emulator_io.h"

#include "utils.h"

#include "main.h"

#include "types.h"

#include "math/math.h"

#include <stdlib.h>

#include <string.h>

#include <setjmp.h>

#include <signal.h>

#define EMULATOR_MULTIBOOT_NAME EMULATOR_INFO_STR

static emulator_t* cur = nullptr;

#ifdef IS_WIN
#define sigsetjmp(__env, __val) setjmp(__env)
#define siglongjmp(__env, __val) longjmp(__env, __val)
#endif

void emulator_setup_tick_timer(emulator_t* _emulator, tick_timer_handler_t handler, _time_t ms) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;

	if (emulator->tick_timers_cnt >= emulator->tick_timers_size) {
		emulator->tick_timers_size += TICK_TIMERS_SIZE_STEP;

		emulator->tick_timers = realloc(emulator->tick_timers, emulator->tick_timers_size * sizeof(tick_timer_t));
 	}

	if (emulator->tick_timers) {
		emulator->tick_timers[emulator->tick_timers_cnt] = (tick_timer_t){ 
			.handler = handler, .ms = ms, .last_time = emulator->ticks
		};
	}

	emulator->tick_timers_cnt += 1;
}

void emulator_release_tick_timer(emulator_t* _emulator, tick_timer_handler_t handler) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;

	_ssize_t index = -1;

	for (_size_t i = 0; i < emulator->tick_timers_cnt; i++) {
		if (emulator->tick_timers[i].handler == handler) {
			index = i; break;
		}
	}

	if (index) {
		emulator->tick_timers[index] = (tick_timer_t){ 0 };
	}
}

void emulator_forced_update_all_timers(emulator_t* _emulator) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;

	if (emulator->tick_timers) {
		for (size_t i = 0; i < emulator->tick_timers_cnt; i++) {
			tick_timer_t* tick_timer = &emulator->tick_timers[i];

			if (!tick_timer) continue;

			tick_timer->handler();
		}
	}
}

void emulator_update_all(emulator_t* _emulator) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;
	
	if (emulator->tick_timers && !emulator->is_hardware_reseting) {
		for (size_t i = 0; i < emulator->tick_timers_cnt; i++) {
			tick_timer_t* tick_timer = &emulator->tick_timers[i];

			if (!tick_timer) continue;

			_time_t dur = emulator->ticks - tick_timer->last_time;

			if (dur >= tick_timer->ms) {
				tick_timer->handler();
				
				tick_timer->last_time = emulator->ticks;
			}
		}
	}

	emulator->ticks += (time_t)(cpu_get_itval_ns(emulator->cpu) / 1000000);
}

static void timer_handler(int sig) {
	if (!cur) return;

	emulator_update_all(cur);
}

static _size_t debug_read() {
	return 0;
}

static void debug_write(_size_t data) {
	return;
}

emulator_t* init_emulator(_ssize_t columns, _ssize_t rows, uint64 frametime_ns, uint64 halted_frametime_ns) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "Emulator initialization...\n");

	emulator_t* emulator = malloc(sizeof(emulator_t));

	memset(emulator, 0, sizeof(emulator_t));

	emulator->tick_timers = malloc(TICK_TIMERS_SIZE_STEP * sizeof(tick_timer_t));

	emulator->tick_timers_cnt = 0; 
	
	emulator->tick_timers_size = TICK_TIMERS_SIZE_STEP;

	emulator->ticks = 0;
	
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);

	emulator->emulator_start_time = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);

	emulator->is_hardware_reseting = true;

	cur = emulator;

	emulator->cpu = init_cpu(frametime_ns, halted_frametime_ns, &timer_handler);

	emulator->ram = init_ram(8 * 1024 * 1024);

	emulator->vga = init_vga_text_screen(emulator->ram, columns, rows);

	emulator->cmos = init_cmos();

	emulator->kbdps2 = init_kbdps2();

	init_power_control();

	emulator_setup_port_in(0x80, debug_read);

	emulator_setup_port_out(0x80, debug_write);

	emulator_log(true, LOG_SEVERITY_INFO, "Emulator initialized\n");

	emulator->is_hardware_reseting = false;

	return emulator;
}

sigjmp_buf emulator_before_kmain_jmp_buf;

// Code 0x20 if exit from OS using power command, or 0x40 if exit using user command
void reset_emulator(emulator_t* emulator, int code) {
	siglongjmp(emulator_before_kmain_jmp_buf, code);
}

void init_emulator_multiboot(emulator_t* _emulator, multiboot_info_t* multiboot_info) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;

	memset(multiboot_info, 0, sizeof(multiboot_info_t));

	memcpy(emulator->ram->mem_ptr + 0x1007C, EMULATOR_MULTIBOOT_NAME, strlen(EMULATOR_MULTIBOOT_NAME));

	memcpy(emulator->ram->mem_ptr + 0x10078, "", 1);

	multiboot_info->boot_loader_name = 0x1007C;

	multiboot_info->boot_device = 0x80FFFFFF;

	multiboot_info->cmdline = 0x10078;

	multiboot_info->flags = 0b0001000000111;

	if (emulator->ram) {
		multiboot_info->mem_lower = MIN(640, emulator->ram->mem_size);

		multiboot_info->mem_upper = (emulator->ram->mem_size - 0x100000) / 1024;

		multiboot_info->flags |= 0x01;
	}

	if (emulator->vga) {
		multiboot_info->fb_width = emulator->vga->width;

		multiboot_info->fb_height = emulator->vga->height;

		multiboot_info->fb_addr = emulator->vga->vidmem_ram_addr;

		multiboot_info->fb_bpp = emulator->vga->bpp;

		multiboot_info->fb_type = FB_TYPE_EGA_TEXT;

		multiboot_info->fb_pitch = emulator->vga->width * emulator->vga->bpp / 8;

		multiboot_info->flags |= 0x1000;
	}
}

void run_emulator(emulator_t* _emulator, void (*kmain)(uint32 magic, multiboot_info_t* multiboot)) {
	if (!kmain) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot run the emulator because no kmain provided to run\n");

		return;
	}

	emulator_log(false, LOG_SEVERITY_INFO, "Emulator running...\n");

	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Multiboot initialization...\n");

	multiboot_info_t multiboot_info = { 0 };

	init_emulator_multiboot(emulator, &multiboot_info);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Multiboot initialized\n");

	// multiboot_info.

	int code = sigsetjmp(emulator_before_kmain_jmp_buf, 1);

	if (code == 0) {
		if (emulator->cpu) emulator->cpu->tsc_start = emulator_read_tsc();

		kmain(0x2BADB002, &multiboot_info);
	}

	else {
		emulator_log(true, LOG_SEVERITY_INFO, "Long jmp code: %i\n\r", code);

		emulator_forced_update_all_timers(emulator);

		emulator->is_hardware_reseting = true;

		reset_kbdps2(emulator->kbdps2);
		
		// reset_cmos(emulator->cmos);
		
		reset_vga_text_screen(emulator->vga);
		
		reset_ram(emulator->ram);
		
		reset_cpu(emulator->cpu);

		emulator->is_hardware_reseting = false;

		if (emulator->cpu) emulator->cpu->tsc_start = emulator_read_tsc();

		kmain(0x2BADB002, &multiboot_info);
	}

	emulator_log(false, LOG_SEVERITY_INFO, "Emulator exited\n");
}

void free_emulator(emulator_t* _emulator) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;

	emulator->is_hardware_reseting = true;

	emulator_forced_update_all_timers(emulator);
	
	if (emulator->kbdps2) {
		free_kbdps2(emulator->kbdps2); emulator->kbdps2 = null;

		release_all_kbdps2();
	}

	if (emulator->cmos) {
		free_cmos(emulator->cmos); emulator->cmos = null;

		release_all_cmos();
	}
	
	if (emulator->vga) {
		release_all_vga_text_screen(emulator->vga); emulator->vga = null;
	}
	
	if (emulator->ram) {
		free_ram(emulator->ram); emulator->ram = null;
	}
	
	if (emulator->cpu) {
		clear_halt(emulator->cpu);

		release_cpu(emulator->cpu); emulator->cpu = null;
	}

	release_power_control();

	free(emulator);

	// fflush(stdout);
}