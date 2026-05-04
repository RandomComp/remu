#include "emulator.h"

#include "cpu/emulator_cpu.h"

#include "time/emulator_pit.h"

#include "memory/emulator_ram.h"

#include "screen/emulator_vga.h"

#include "time/emulator_cmos.h"

#include "hid/emulator_kbdps2.h"

#include "ata/emulator_hdd_pio.h"

#include "power/emulator_power_control.h"

#include "emulator_multiboot.h"

#include "emulator_io.h"

#include "emulator_logger.h"

#include "utils.h"

#include "main.h"

#include "types.h"

#include "math/math.h"

#include <stdlib.h>

#include <string.h>

#include <setjmp.h>

#include <signal.h>

#include <pthread.h>

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL.h>
#endif

#define EMULATOR_MULTIBOOT_NAME EMULATOR_INFO_STR

static emulator_t* cur = nullptr;

tick_timer_t emulator_setup_tick_timer(emulator_t* _emulator, tick_timer_handler_t handler, time_t ms) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	tick_timer_t timer = (tick_timer_t){ 
		.handler = handler, .ms = ms, .last_time = emulator->ticks
	};

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot setup timer: no emulator instance provided");
		
		return timer;
	}

	if (emulator->tick_timers_cnt >= emulator->tick_timers_size) {
		emulator->tick_timers_size += TICK_TIMERS_SIZE_STEP;

		emulator->tick_timers = realloc(emulator->tick_timers, emulator->tick_timers_size * sizeof(tick_timer_t));
 	}

	if (emulator->tick_timers) {
		emulator->tick_timers[emulator->tick_timers_cnt] = timer;

		emulator->tick_timers_cnt += 1;
	}

	return timer;
}

void emulator_release_tick_timer(emulator_t* _emulator, tick_timer_handler_t handler) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator || !emulator->tick_timers || emulator->cleaned || !handler) return;

	_ssize_t index = -1;

	_size_t tick_timers_cnt = MIN(emulator->tick_timers_cnt, emulator->tick_timers_size);

	for (_size_t i = 0; i < tick_timers_cnt; i++) {
		if (emulator->tick_timers[i].handler == handler) {
			index = i; break;
		}
	}

	if (index >= 0) {
		emulator->tick_timers[index] = (tick_timer_t){ 0 };
	}
}

void emulator_forced_update_all_timers(emulator_t* _emulator) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;

	if (emulator->tick_timers) {
		for (size_t i = 0; i < emulator->tick_timers_cnt; i++) {
			tick_timer_t tick_timer = emulator->tick_timers[i];

			if (tick_timer.handler) tick_timer.handler();
		}
	}
}

void emulator_update_timer(tick_timer_t* tick_timer, uint64 cur_ticks) {
	if (!tick_timer || !tick_timer->handler) {
		emulator_log(false, LOG_SEVERITY_WARNING, "invalid tick timer pointer present");

		return;
	}

	time_t dur = cur_ticks - tick_timer->last_time;

	if (dur >= tick_timer->ms) {
		tick_timer->handler();
					
		tick_timer->last_time = cur_ticks;
	}
}

void emulator_update_all(emulator_t* _emulator) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot update all timers: no emulator instance provided");

		return;
	}

	if (!emulator->tick_timers) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot update all timers: no emulator instance provided");

		return;
	}
	
	if (!emulator->is_hardware_reseting) {
		for (size_t i = 0; i < emulator->tick_timers_cnt; i++) {
			tick_timer_t* tick_timer = &emulator->tick_timers[i];

			emulator_update_timer(tick_timer, emulator->ticks);

			time_t dur = emulator->ticks - tick_timer->last_time;

			if (dur >= tick_timer->ms) {
				tick_timer->last_time = emulator->ticks;
			} 
		}
	}

	emulator->ticks += (time_t)(cpu_get_itval_ns(emulator->cpu) / 1000000);
}

static _size_t debug_read() {
	return 0;
}

static void debug_write(_size_t data) {
	return;
}

emulator_t* init_emulator(bool gui, _ssize_t width, _ssize_t height, uint64 frametime_ns, uint64 halted_frametime_ns) {
	emulator_log(true, LOG_SEVERITY_INFO, "Emulator initialization...");

	emulator_t* emulator = malloc(sizeof(emulator_t));

	memset(emulator, 0, sizeof(emulator_t));

	if (gui) {
		// TODO: Исправить холостое открытие при SDL_CreateWindow

		#ifdef EMULATOR_SDL_USING
		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL window and renderer initialization...");

		emulator->window = SDL_CreateWindow("OS Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);

		if (!emulator->window) {
			emulator_log(true, LOG_SEVERITY_ERROR, "SDL window initialization error: %s", SDL_GetError());

			free_emulator(emulator);
			
			return nullptr;
		}

		SDL_SetWindowMinimumSize(emulator->window, width, height);

		SDL_SetWindowMaximumSize(emulator->window, width * 2, height * 2);

		emulator->renderer = SDL_CreateRenderer(emulator->window, -1, SDL_RENDERER_ACCELERATED);
		
		if (!emulator->renderer) {
			emulator_log(true, LOG_SEVERITY_ERROR, "SDL renderer initialization error: %s", SDL_GetError());

			free_emulator(emulator);

			return nullptr;
		}

		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL window and renderer initialized!");
		
		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL texture initialization (for drawing)...");

		emulator->screen_width =  80 * VGA_CHAR_WIDTH;

		emulator->screen_height = 25 * VGA_CHAR_HEIGHT;

		emulator->screen_texture = SDL_CreateTexture(emulator->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, emulator->screen_width, emulator->screen_height);

		if (!emulator->screen_texture) {
			emulator_log(true, LOG_SEVERITY_ERROR, "SDL texture initialization error: %s", SDL_GetError());

			free_emulator(emulator);

			return nullptr;
		}

		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL texture framebuffer initialization...");

		emulator->screen = malloc(emulator->screen_width * emulator->screen_height * sizeof(uint32));

		if (!emulator->screen) {
			emulator_log(true, LOG_SEVERITY_ERROR, "SDL texture framebuffer initialization error");

			free_emulator(emulator);

			return nullptr;
		}

		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL texture framebuffer initialized!");

		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL texture initialized!");

		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL initialized!");
		#endif
	}

	emulator->tick_timers = malloc(TICK_TIMERS_SIZE_STEP * sizeof(tick_timer_t));

	emulator->tick_timers_cnt = 0; 
	
	emulator->tick_timers_size = TICK_TIMERS_SIZE_STEP;

	emulator->ticks = 0;

	emulator->gui = gui;
	
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);

	emulator->emulator_start_time = (ts.tv_nsec / 1000000) + (ts.tv_sec * 1000);

	emulator->is_hardware_reseting = true;

	cur = emulator;

	emulator->cpu = init_cpu(frametime_ns, halted_frametime_ns);

	emulator->ram = init_ram(8 * 1024 * 1024);

	emulator->pit = init_pit(emulator->cpu->pic);

	setup_pit(emulator);

	#ifdef EMULATOR_SDL_USING
	emulator->vga = init_vga_text_screen(
		emulator->screen_texture, emulator->screen, emulator->screen_width, emulator->screen_height, 
		emulator->gui, emulator->ram, 80, 25
	);
	#else
	emulator->vga = init_vga_text_screen(
		emulator->ram, 80, 25
	);
	#endif

	emulator->cmos = init_cmos();

	emulator->kbdps2 = init_kbdps2(emulator->gui);

	emulator->hdd = init_hdd_ata_pio(32 * 2); // 32 KB

	init_power_control();

	emulator_setup_port_in(0x80, debug_read);

	emulator_setup_port_out(0x80, debug_write);

	emulator_log(true, LOG_SEVERITY_INFO, "Emulator initialized");

	emulator->is_hardware_reseting = false;

	return emulator;
}

// Code 0x20 if exit from OS using power command, or 0x40 if exit using user command
void reset_emulator(emulator_t* emulator, int code) {
	emulator->running = false;
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
		multiboot_info->mem_lower = MIN(640, emulator->ram->mem_size / 1024);

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

struct kmain_start_args_t {
	void (*kmain)(uint32 magic, multiboot_info_t* multiboot);

	uint32 magic;

	multiboot_info_t* multiboot;
};

static void exec_ints_timer_handler(int UNUSED sig) {
	if (!cur || !cur->cpu || !cur->cpu->pic) return;

	exec_all_emulator_ints(cur->cpu->pic);
}

// #include <unistd.h>

static void* kmain_start(void* args) {
	if (!cur || !(cur->cpu) || !args) {
		if (args) free(args);

		return nullptr;
	}

	struct kmain_start_args_t* start_args = (struct kmain_start_args_t*)args;

	uint32 magic = start_args->magic;

	multiboot_info_t* multiboot = start_args->multiboot;

	void (*kmain)(uint32 magic, multiboot_info_t *multiboot) = start_args->kmain;

	if (!kmain) {
		free(start_args); return nullptr;
	}

	free(start_args);

	init_timer(&(cur->kmain_ints_exec_timer), cur->cpu->halted_frametime_ns / 2, exec_ints_timer_handler);

	if (kmain)
		kmain(magic, multiboot);

	// sleep(5);
	
	timer_delete(cur->kmain_ints_exec_timer); cur->kmain_ints_exec_timer = 0;

	cur->running = false;

	cur->kmain_started = false;
	
	return nullptr;
}

pthread_t run_emulator(emulator_t* _emulator, void (*kmain)(uint32 magic, multiboot_info_t* multiboot)) {
	if (!kmain) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot run the emulator because no kmain provided to run");

		return 0;
	}

	emulator_log(false, LOG_SEVERITY_INFO, "Emulator running...");

	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return 0;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Multiboot initialization...");

	emulator->multiboot_info = malloc(sizeof(multiboot_info_t));

	init_emulator_multiboot(emulator, emulator->multiboot_info);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Multiboot initialized");

	if (emulator->cpu) emulator->cpu->tsc_start = emulator_read_tsc();

	struct kmain_start_args_t* start_args = malloc(sizeof(struct kmain_start_args_t));

	start_args->kmain = kmain;
	start_args->magic = 0x2BADB002;
	start_args->multiboot = emulator->multiboot_info;

	pthread_t kmain_thread;

	int code = pthread_create(&kmain_thread, null, kmain_start, start_args);

	if (code != 0) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Creating thread for kmain function (%x) error: %i", kmain, code);

		return 0;
	}

	emulator->kmain_thread = kmain_thread;

	emulator->running = true;

	emulator->kmain_started = true;

	return kmain_thread;
}

void free_emulator(emulator_t* _emulator) {
	emulator_t* emulator = _emulator;

	if (!_emulator && cur) emulator = cur;

	if (!emulator) return;

	emulator->is_hardware_reseting = true;

	emulator_forced_update_all_timers(emulator);

	if (cur->kmain_ints_exec_timer > 0)
		timer_delete(cur->kmain_ints_exec_timer);
		 
	cur->kmain_ints_exec_timer = 0;
	
	if (emulator->pit) {
		free_pit(emulator->pit); emulator->pit = nullptr;

		release_pit(emulator, emulator->pit);
	}
	
	if (emulator->cpu) {
		clear_halt(emulator->cpu);

		release_cpu(emulator->cpu); emulator->cpu = nullptr;
	}

	if (emulator->kbdps2) {
		free_kbdps2(emulator->kbdps2); emulator->kbdps2 = nullptr;

		release_all_kbdps2();
	}

	if (emulator->hdd) {
		free_hdd_ata_pio(emulator->hdd); emulator->hdd = nullptr;
	}

	if (emulator->cmos) {
		free_cmos(emulator->cmos); emulator->cmos = nullptr;

		release_all_cmos();
	}
	
	if (emulator->vga) {
		release_all_vga_text_screen(emulator->vga); emulator->vga = nullptr;
	}
	
	if (emulator->ram) {
		free_ram(emulator->ram); emulator->ram = nullptr;
	}
	
	if (emulator->tick_timers) {
		free(emulator->tick_timers); emulator->tick_timers = nullptr;

		emulator->tick_timers_cnt = 0; emulator->tick_timers_size = 0;
	}
	
	release_power_control();

	if (emulator->multiboot_info) {
		free(emulator->multiboot_info); emulator->multiboot_info = nullptr;
	}

	#ifdef EMULATOR_SDL_USING
	if (emulator->screen) {
		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL texture framebuffer deinitialization...");

		free(emulator->screen);
		
		emulator->screen = nullptr;
		
		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL texture framebuffer deinitialized!");
	}

	if (emulator->screen_texture) {
		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL texture deinitialization...");

		SDL_DestroyTexture(emulator->screen_texture);
		
		emulator->screen_texture = null;

		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL texture deinitialized!");
	}

	if (emulator->renderer) {
		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL renderer deinitialization...");

		SDL_DestroyRenderer(emulator->renderer);
		
		emulator->renderer = null;
		
		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL renderer deinitialized!");
	}

	if (emulator->window) {
		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL window deinitialization...");

		SDL_DestroyWindow(emulator->window);
		
		emulator->window = null;

		emulator_log(true, LOG_SEVERITY_VERBOSE, "SDL window deinitialized!");
	}
	#endif

	emulator->cleaned = true;
		
	free(emulator);

	if (cur == emulator) cur = nullptr;
}
