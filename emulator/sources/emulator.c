#include "emulator.h"

#include "cpu/emulator_cpu.h"

#include "pci/emulator_pci.h"

#include "time/emulator_pit.h"

#include "memory/emulator_ram.h"

#include "screen/emulator_vga_gpu.h"

#include "screen/emulator_vga_screen.h"

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

#include <signal.h>

#include <pthread.h>

#include <dlfcn.h>

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL.h>
#endif

#ifdef IS_UNIX
#include <unistd.h>

#include <sys/time.h>
#endif

#define EMULATOR_MULTIBOOT_NAME EMULATOR_INFO_STR

static emulator_t* cur = nullptr;

tick_timer_t emulator_setup_tick_timer(emulator_t* _emulator, tick_timer_handler_t handler, time_t ms) {
	emulator_t* emulator = _emulator;

	if (!emulator) emulator = cur;

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot setup tick timer: no emulator instance present");

		return (tick_timer_t){ 0 };
	}

	// TODO: ms * 2 -- ╨╕╤ü╨┐╤Ç╨░╨▓╨╕╤é╤î ╨▓╤Ç╨╡╨╝╨╡╨╜╨╜╨╛╨╡ ╤Ç╨╡╤ê╨╡╨╜╨╕╨╡
	tick_timer_t timer = (tick_timer_t){ 
		.handler = handler, .ms = ms * 2, .last_time = emulator->ticks
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

	if (!emulator) emulator = cur;

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
			tick_timer_t* tick_timer = &(emulator->tick_timers[i]);

			emulator_update_timer(tick_timer, emulator->ticks);
		}
	}

	emulator->ticks += (time_t)(cpu_get_itval_ns(emulator->cpu) / 1000000);
}

static _size_t debug_read() {
	return 0;
}

static byte buf[64] = { 0 }; static size_t buf_pos = 0;

static void debug_write(_size_t data) {
	data = data & 0x7F;

	if (data == '\n' || buf_pos >= sizeof(buf)) {
		emulator_log(true, LOG_SEVERITY_REPORT, "%.64s", buf);

		memset(buf, 0, sizeof(buf));

		buf_pos = 0;
	}

	else buf[buf_pos++] = data;

	return;
}

emulator_t* init_emulator(bool gui, _ssize_t width, _ssize_t height, _ssize_t frame_width, _ssize_t frame_height, _ssize_t bpp, bool vesa_mode, uint32 frametime_ns, uint32 halted_frametime_ns) {
	emulator_log(true, LOG_SEVERITY_INFO, "Emulator initialization...");

	if (vesa_mode) {
		#ifndef EMULATOR_SDL_USING
		emulator_log(true, LOG_SEVERITY_ERROR, "VESA is unsupported in non-gui mode");

		return nullptr;
		#endif
	}

	emulator_t* emulator = malloc(sizeof(emulator_t));

	memset(emulator, 0, sizeof(emulator_t));

	if (gui) {
		// TODO: ╨ÿ╤ü╨┐╤Ç╨░╨▓╨╕╤é╤î ╤à╨╛╨╗╨╛╤ü╤é╨╛╨╡ ╨╛╤é╨║╤Ç╤ï╤é╨╕╨╡ ╨┐╤Ç╨╕ SDL_CreateWindow

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

		if (vesa_mode) {
			emulator->screen_width =  frame_width;
			emulator->screen_height = frame_height;
		}

		else {
			emulator->screen_width =  frame_width * VGA_CHAR_WIDTH;
			emulator->screen_height = frame_height * VGA_CHAR_HEIGHT;
		}

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

	emulator->pci = init_pci();

	emulator->ram = init_ram(8 * 1024 * 1024);

	emulator->pit = init_pit(emulator->cpu->pic);

	setup_pit(emulator);

	if (vesa_mode) {
		#ifndef EMULATOR_SDL_USING
		emulator_log(true, LOG_SEVERITY_ERROR, "VESA is unsupported in non-gui mode");

		free_emulator(emulator);

		return nullptr;
		#else
		emulator->vesa_gpu = init_vesa_device(
			frame_width, frame_height, bpp
		);

		emulator->vesa_screen = init_vesa_screen(
			emulator->screen_texture, emulator->screen,
			emulator->screen_width, emulator->screen_height, 
			emulator->vesa_gpu
		);
		#endif
	}

	else {
		emulator->vga_gpu = init_vga_text_device(
			emulator->ram, frame_width, frame_height
		);

		#ifdef EMULATOR_SDL_USING
		emulator->vga_screen = init_vga_text_screen(
			emulator->vga_gpu, 
			emulator->screen_texture, emulator->screen, 
			emulator->screen_width, emulator->screen_height, 
			emulator->gui
		);
		#else
		emulator->vga_screen = init_vga_text_screen(
			emulator->vga_gpu
		);
		#endif
	}

	emulator->cmos = init_cmos();

	emulator->kbdps2 = init_kbdps2(emulator->gui);

	emulator->hdd = init_hdd_ata_pio(2 * 1024 * 2); // 2 MB

	init_power_control();

	emulator->is_hardware_reseting = false;

	emulator_setup_port_in(0x80, debug_read);
	emulator_setup_port_out(0x80, debug_write);

	emulator_log(true, LOG_SEVERITY_INFO, "Emulator initialized");

	return emulator;
}

// Code 0x20 if exit from OS using power command, or 0x40 if exit using user command
void reset_emulator(emulator_t* _emulator, int code) {
	emulator_log(true, LOG_SEVERITY_INFO, "Emulator reseting...");

	emulator_t* emulator = _emulator;

	if (!emulator) emulator = cur;

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot reset emulator: invalid emulator instance");

		return;
	}

	stop_emulator(emulator);

	emulator->running = true;
}

static void* emulator_get_ram() {
	if (!cur) {
		emulator_log(true, LOG_SEVERITY_ERROR, "[CALLING FROM KERNEL] Cannot get the emulator ram: emulator not initialized");

		return nullptr;
	}

	return cur->ram->mem_ptr;
}

static bool cpu_need_to_halt = false;

static uint64 itval_ns = 0;

static void wait_halt() {
	usleep(itval_ns / 1000);

	cpu_need_to_halt = true;
}

static void set_sti(void) {
	if (!cur || !cur->cpu || !cur->cpu->pic) return;

	set_sti_pic(cur->cpu->pic);
}

static void set_cli(void) {
	if (!cur || !cur->cpu || !cur->cpu->pic) return;

	set_cli_pic(cur->cpu->pic);
}

static uint64 start_tsc() {
	return cur && cur->cpu ? cur->cpu->tsc_start : 0;
}

void main_loop(emulator_t* _emulator, multiboot_section_t* multiboot_section) {
	emulator_t* emulator = _emulator;

	if (!emulator) emulator = cur;

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot run main loop: invalid emulator instance");
		
		return;
	}

	if (!multiboot_section) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot run main loop: invalid multiboot section pointer");
		
		return;
	}

	#ifdef EMULATOR_SDL_USING
	
	SDL_Event event;

	int win_x = 0, win_y = 0;

	while (emulator && emulator->kernel->kmain_started) {
		bool need_exit = false;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because pressed quit button...");

				emulator->running = false; need_exit = true;
				
				break;
			}

			if (event.type == SDL_KEYDOWN) {
				bool is_ctrl = (event.key.keysym.mod & KMOD_CTRL) || (event.key.keysym.mod & KMOD_GUI);
				bool is_shift = (event.key.keysym.mod & KMOD_SHIFT);

				if (multiboot_section->mode_type != 0) {
					if (is_ctrl && is_shift) {
						switch (event.key.keysym.sym) {
							case SDLK_c:
								handle_copy_selected();
								break;
							case SDLK_v:
								handle_paste_selected();
								break;
							
							default:
								break;
						}
					}

					else
						handle_key_gui(event.key.keysym.scancode, false);
				}

				else
					handle_key_gui(event.key.keysym.scancode, false);
			}

			else if (event.type == SDL_KEYUP) {
				handle_key_gui(event.key.keysym.scancode, true);
			}

			if (multiboot_section->mode_type != 0) {
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					if (event.button.button == 1) {
						SDL_GetWindowSize(emulator->window, &win_x, &win_y);

						handle_mouse_button(event.motion.x, event.motion.y, win_x, win_y, false);
					}
				}

				else if (event.type == SDL_MOUSEBUTTONUP) {
					if (event.button.button == 1) {
						SDL_GetWindowSize(emulator->window, &win_x, &win_y);

						handle_mouse_button(event.motion.x, event.motion.y, win_x, win_y, true);
					}
				}

				else if (event.type == SDL_MOUSEMOTION) {
					SDL_GetWindowSize(emulator->window, &win_x, &win_y);

					handle_mouse_move(event.motion.x, event.motion.y, win_x, win_y);
				}
			}
		}

		if (need_exit) break;

		// TODO: ╨ö╨╛╨┐╨╕╤ü╨░╤é╤î ╨╕╤ü╨┐╨╛╨╗╤î╨╖╨╛╨▓╨░╨╜╨╕╨╡ SDL_GetKeyboardState ╨┤╨╗╤Å ╨│╨╡╨╜╨╡╤Ç╨░╤å╨╕╨╕ ╤ü╨╛╨▒╤ï╤é╨╕╨╣ ╨║╨╗╨░╨▓╨╕╤ê PS/2

		// int numkeys = 0;

		// uint8* kbd_states = SDL_GetKeyboardState(&numkeys);

		// for (int i = 0; i < numkeys; i++) {
			
		// }

		if (!emulator->is_hardware_reseting)
			emulator_update_all(emulator);

		SDL_SetRenderDrawColor(emulator->renderer, 0, 0, 0, 255);
		SDL_RenderClear(emulator->renderer);

		// TODO: ╨╛╨▒╨╜╨╛╨▓╨╗╤Å╤é╤î ╤ì╨║╤Ç╨░╨╜ ╤é╨╛╨╗╤î╨║╨╛ ╨┐╤Ç╨╕ ╨╜╨╡╨╛╨▒╤à╨╛╨┤╨╕╨╝╨╛╤ü╤é╨╕

		SDL_RenderCopy(emulator->renderer, emulator->screen_texture, null, null);

		SDL_RenderPresent(emulator->renderer);

		if (!(emulator->cpu->halted) && cpu_need_to_halt) {
			set_halt(emulator->cpu);
		}
			
		itval_ns = cpu_get_itval_ns(emulator->cpu);

		// SDL_Delay((uint32)(itval_ns / 1000000));

		// usleep(itval_ns / 1000);

		usleep(cpu_get_itval_ns(emulator->cpu) / 1000);

		// TODO: ╨í╨┤╨╡╨╗╨░╤é╤î ╨╕╨╖╨╝╨╡╤Ç╨╡╨╜╨╕╨╡ ╤Ç╨╡╨░╨╗╤î╨╜╨╛╨│╨╛ ╨▓╤Ç╨╡╨╝╨╡╨╜╨╕ ╨╖╨░╨┤╨╡╤Ç╨╢╨║╨╕ ╨╕ ╨░╨┤╨░╨┐╤é╨╕╤Ç╨╛╨▓╨░╨╜╨╕╨╡

		// TODO: ╨ÿ╤ü╨┐╤Ç╨░╨▓╨╕╤é╤î ╨▒╨░╨│ ╤ü ╤â╤ü╨║╨╛╤Ç╨╡╨╜╨╕╨╡╨╝ ╨▓ ╨┤╨▓╨░ ╤Ç╨░╨╖╨░
	}
	#else
	emulator_setup_tick_timer(emulator, handle_keys_cli, 10);

	while (emulator && emulator->running) {
		if (!emulator->is_hardware_reseting)
			emulator_update_all(emulator);

		if (!(emulator->cpu->halted) && cpu_need_to_halt) {
			set_halt(emulator->cpu);
		}
			
		itval_ns = cpu_get_itval_ns(emulator->cpu);

		usleep(itval_ns / 1000);
	}
	#endif
}

kernel_t* emulator_load_kernel(const byte* filename) {
	emulator_log(true, LOG_SEVERITY_INFO, "Kernel \"%s\" loading...", filename);

	void* os_handle = dlopen(filename, RTLD_NOW);

	if (!os_handle) {
		emulator_log(true, LOG_SEVERITY_ERROR, "dlopen error: %s.", dlerror());

		return nullptr;
	}

	kernel_t* kernel = malloc(sizeof(kernel_t));

	kernel->__emulator_init_kernel = dlsym(os_handle, "__emulator_init_kernel");
	
	if (!kernel->__emulator_init_kernel) {
		emulator_log(true, LOG_SEVERITY_ERROR, "No such function named \"__emulator_init_kernel\" in provided \"%s\". Please verify the function name.", filename);

		dlclose(os_handle);

		free(kernel);

		return nullptr;
	}

	kernel->__emulator_read_multiboot_secton = dlsym(os_handle, "__emulator_read_multiboot_secton");
	
	if (!kernel->__emulator_read_multiboot_secton) {
		emulator_log(true, LOG_SEVERITY_ERROR, "No such function named \"__emulator_read_multiboot_secton\" in provided \"%s\". Please verify the function name.", filename);

		dlclose(os_handle);

		free(kernel);

		return nullptr;
	}

	kernel->kmain = dlsym(os_handle, "kmain");

	if (!kernel->kmain) {
		emulator_log(true, LOG_SEVERITY_ERROR, "No such function named \"kmain\" in provided \"%s\". Please verify the function name.", filename);

		dlclose(os_handle);

		free(kernel);

		return nullptr;
	}

	emulator_log(true, LOG_SEVERITY_INFO, "Kernel \"%s\" loaded", filename);

	emulator_log(true, LOG_SEVERITY_INFO, "Kernel initializing...");

	__init_kernel_args_t kernel_args = { 0 };

	kernel_args.__emulator_get_ram = emulator_get_ram;
	kernel_args.__emulator_port_in = port_in;
	kernel_args.__emulator_port_out = port_out;
	kernel_args.__emulator_wait_halt = wait_halt;
	kernel_args.__emulator_sti = set_sti;
	kernel_args.__emulator_cli = set_cli;
	kernel_args.__emulator_start_tsc = start_tsc;
	kernel_args.__emulator_idt_flush = idt_flush_emulator;

	kernel->__emulator_init_kernel(&kernel_args);

	emulator_log(true, LOG_SEVERITY_INFO, "Kernel initialized");

	return kernel;
}

int emulator_unload_kernel(emulator_t* _emulator) {
	emulator_log(true, LOG_SEVERITY_INFO, "Kernel unloading...");

	emulator_t* emulator = _emulator;

	if (!emulator) emulator = cur;

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot unload kernel: invalid emulator instance");

		return 1;
	}

	if (!emulator->kernel->dl_handle) {
		return 0;
	}

	int err = dlclose(emulator->kernel->dl_handle);
	
	emulator->kernel->dl_handle = nullptr;

	if (err != 0) {
		emulator_log(true, LOG_SEVERITY_ERROR, "dlopen error: %s.", dlerror());

		return 1;
	}

	emulator_log(true, LOG_SEVERITY_INFO, "Kernel unloaded");

	return 0;
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

	if (emulator->vga_gpu) {
		multiboot_info->fb_width = emulator->vga_gpu->width;

		multiboot_info->fb_height = emulator->vga_gpu->height;

		multiboot_info->fb_addr = emulator->vga_gpu->vidmem_ram_addr;

		multiboot_info->fb_bpp = emulator->vga_gpu->bpp;

		multiboot_info->fb_type = FB_TYPE_EGA_TEXT;

		multiboot_info->fb_pitch = emulator->vga_gpu->width * emulator->vga_gpu->bpp / 8;

		multiboot_info->flags |= 0x1000;
	}

	if (emulator->vesa_gpu) {
		multiboot_info->fb_width = emulator->vesa_gpu->width;
		multiboot_info->fb_height = emulator->vesa_gpu->height;
		multiboot_info->fb_bpp = emulator->vesa_gpu->bpp;

		multiboot_info->fb_addr = (uint64)(emulator->vesa_gpu->vidmem);

		multiboot_info->fb_type = FB_TYPE_RGB;

		multiboot_info->fb_pitch = emulator->vesa_gpu->width * emulator->vesa_gpu->bpp / 8;

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

	init_timer(&(cur->kernel->kmain_ints_exec_timer), cur->cpu->halted_frametime_ns / 2, exec_ints_timer_handler);

	if (kmain)
		kmain(magic, multiboot);

	// sleep(5);
	
	timer_delete(cur->kernel->kmain_ints_exec_timer); cur->kernel->kmain_ints_exec_timer = 0;

	cur->running = false;

	cur->kernel->kmain_started = false;
	
	return nullptr;
}

pthread_t run_emulator(emulator_t* _emulator) {
	emulator_t* emulator = _emulator;

	if (!emulator) emulator = cur;

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot run emulator: invalid emulator instance");

		return 0;
	}

	if (!emulator->kernel || !emulator->kernel->kmain) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot run emulator: no kmain provided to run");

		return 0;
	}

	emulator_log(false, LOG_SEVERITY_INFO, "Emulator running...");

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Multiboot initialization...");

	emulator->multiboot_info = malloc(sizeof(multiboot_info_t));

	init_emulator_multiboot(emulator, emulator->multiboot_info);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Multiboot initialized");

	if (emulator->cpu)
		emulator->cpu->tsc_start = emulator_read_tsc();

	struct kmain_start_args_t* start_args = malloc(sizeof(struct kmain_start_args_t));

	start_args->kmain = 	emulator->kernel->kmain;
	start_args->magic = 	0x2BADB002;
	start_args->multiboot = emulator->multiboot_info;

	pthread_t kmain_thread;

	int code = pthread_create(&kmain_thread, null, kmain_start, start_args);

	if (code != 0) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Creating thread for kmain function (%x) error: %i", emulator->kernel->kmain, code);

		return 0;
	}

	emulator_log(true, LOG_SEVERITY_VERBOSE, "Created thread for kmain function (%x)", emulator->kernel->kmain);

	emulator->kernel->kmain_thread = kmain_thread;

	emulator->running = true;

	emulator->kernel->kmain_started = true;

	return kmain_thread;
}

void stop_emulator(emulator_t* emulator) {
	if (emulator->kernel) {
		if (emulator->kernel->kmain_ints_exec_timer > 0) {
			timer_delete(cur->kernel->kmain_ints_exec_timer); cur->kernel->kmain_ints_exec_timer = 0;
		}

		if (emulator->kernel->kmain_thread > 0) {
			pthread_cancel(emulator->kernel->kmain_thread);

			emulator->kernel->kmain_thread = 0;
		}

		if (emulator->multiboot_info) {
			free(emulator->multiboot_info); emulator->multiboot_info = nullptr;
		}

		emulator->kernel->kmain_started = false;
	}

	emulator->running = false;
}

void free_emulator(emulator_t* _emulator) {
	emulator_t* emulator = _emulator;

	if (!emulator) emulator = cur;

	if (!emulator) return;

	emulator->is_hardware_reseting = true;

	emulator_forced_update_all_timers(emulator);

	stop_emulator(emulator);

	emulator->running = false;
	
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
	
	if (emulator->vesa_screen) {
		free_vesa_screen(emulator->vesa_screen); emulator->vesa_screen = nullptr;
	}
	
	if (emulator->vesa_gpu) {
		free_vesa_device(emulator->vesa_gpu); emulator->vesa_gpu = nullptr;
	}
	
	if (emulator->vga_screen) {
		release_all_vga_text_screen(emulator->vga_screen); emulator->vga_screen = nullptr;
	}
	
	if (emulator->vga_gpu) {
		release_all_vga_text_device(emulator->vga_gpu); emulator->vga_gpu = nullptr;
	}
	
	if (emulator->ram) {
		free_ram(emulator->ram); emulator->ram = nullptr;
	}
	
	if (emulator->pci) {
		free_pci(emulator->pci); emulator->pci = nullptr;
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
