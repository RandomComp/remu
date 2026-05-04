#include "main.h"

// Код этого эмулятора 0xEA32

#include "types.h"

#include <stdio.h>

#include <stdlib.h>

#ifdef IS_UNIX
#include <unistd.h>

#include <fcntl.h>

#include <sys/time.h>
#endif

#include <time.h>

#include <string.h>

#include "ansi.h"

#include "math.h"

#include "utils.h"

#include "emulator.h"

#include "emulator_logger.h"

#include <pthread.h>

#include <signal.h>

#include <dlfcn.h>

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#endif

#ifdef IS_UNIX
struct termios* orig_termios;
#endif

static emulator_t* emulator = nullptr;

logger_t* logger = nullptr;

// TODO: Сделать двойной буфер видеопамяти

// TODO: Сделать таблицу прерываний и их настройку

static void on_emulator_exit() {
	if (orig_termios) {
		switch_to_default(orig_termios);

		free(orig_termios);

		printf(visible_cursor);
	}

	printf("\n\r");

	// imd_exit_emulator(0);
}

void exit_emulator(int code) {
	// uint64 start_cpu_tsc = (emulator && emulator->cpu) ? emulator->cpu->tsc_start : emulator_read_tsc();

	// uint64 working_cpu_tsc = emulator_read_tsc() - start_cpu_tsc;

	// uint64 start_time = emulator ? emulator->emulator_start_time : 0;

	// struct timespec ts;

	// timespec_get(&ts, TIME_UTC);

	// uint64 emulator_working_time = ((ts.tv_nsec / 1000000) + (ts.tv_sec * 1000)) - start_time;

	// uint64 ticks = emulator ? emulator->ticks : 0;

	// emulator_log(true, LOG_SEVERITY_INFO, "Uptime: %llu ms", emulator_working_time);
	
	// emulator_log(true, LOG_SEVERITY_INFO, "Timer ticks: %li (1 tick is ~1 ms)", ticks);
	
	// emulator_log(true, LOG_SEVERITY_INFO, "CPU TSC since starting: %llu", working_cpu_tsc);

	// if (emulator) free_emulator(emulator); emulator = nullptr;

	// printf(visible_cursor);

	// if (log_file) fclose(log_file);
	
	// log_file = nullptr;

	// #ifdef EMULATOR_SDL_USING
	// IMG_Quit();
	// SDL_Quit();
	// #endif

	// exit(code);

	if (emulator) emulator->running = false;
}

void imd_exit_emulator(int code) {
	if (emulator) {
		emulator->running = false;

		if (emulator->kmain_thread > 0)
			pthread_cancel(emulator->kmain_thread);
		
		emulator->kmain_thread = 0;

		#ifndef EMULATOR_SDL_USING
		emulator_release_tick_timer(emulator, handle_keys_cli);
		#endif

		free_emulator(emulator);

		#ifdef EMULATOR_SDL_USING
		IMG_Quit();
		SDL_Quit();
		#endif

		emulator = nullptr;
	}

	if (logger) free_emulator_logger(logger);

	logger = nullptr;
}

// extern void printf(const char* format, int len);

// extern void exit123(int);

typedef struct __init_kernel_args_t {
	void* (*__emulator_get_ram)(void);

	_size_t (*__emulator_port_in)(uint16 port);

	void (*__emulator_port_out)(uint16 port, _size_t value);

	void (*__emulator_wait_halt)(void);

	void (*__emulator_sti)(void);

	void (*__emulator_cli)(void);

	uint64 (*__emulator_start_tsc)(void);

	void (*__emulator_idt_flush)(idt_ptr_t* ptr);

	void (*__emulator_kernel_report)(const c_str msg);
} __init_kernel_args_t;

static void* emulator_get_ram() {
	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "[CALLING FROM KERNEL] Cannot get the emulator ram: emulator not initialized");

		return nullptr;
	}

	return emulator->ram->mem_ptr;
}

static bool cpu_need_to_halt = false;

static uint64 itval_ns = 0;

static void wait_halt() {
	usleep(itval_ns / 1000);

	cpu_need_to_halt = true;
}

static void set_sti(void) {
	if (!emulator || !emulator->cpu || !emulator->cpu->pic) return;

	set_sti_pic(emulator->cpu->pic);
}

static void set_cli(void) {
	if (!emulator || !emulator->cpu || !emulator->cpu->pic) return;

	set_cli_pic(emulator->cpu->pic);
}

static uint64 start_tsc() {
	return emulator && emulator->cpu ? emulator->cpu->tsc_start : 0;
}

static void report(const c_str msg) {
	emulator_log(true, LOG_SEVERITY_REPORT, msg ? msg : "No message provided.");

	// free_emulator(emulator);

	// exit(0xBD);

	// #ifdef EMULATOR_SDL_USING
	// IMG_Quit();
	// SDL_Quit();
	// #endif
}

int main(int argc, const char** argv) {
	#ifdef IS_WIN
	printf("This emulator now is not fully supported on windows, use it for your own risk\n\r");
	#endif

	logger = init_emulator_logger();

	if (argc > 2) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Too many arguments. Expected only one argument: kernel ./*.so file");

		free_emulator_logger(logger);

		return 1;
	}

	else if (argc < 2) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Too few arguments. Expected only one argument: kernel ./*.so file");

		free_emulator_logger(logger);

		return 1;
	}

	const c_str so_name = argv[1];

	atexit(on_emulator_exit);

	#ifdef EMULATOR_SDL_USING
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize SDL video subsystem: %s", SDL_GetError());

		return 1;
	}

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize SDL IMG subsystem (for loading font): %s", IMG_GetError());

		SDL_Quit();

		return 1;
	}

	#endif

	time_t cur_time = time(0);

	struct tm* local_time = localtime(&cur_time);

	char buf[32];

	strftime(buf, 32, "%d %B %Y", local_time);

	emulator_log(true, LOG_SEVERITY_INFO, EMULATOR_INFO_FULL_STR, PLATFORM_COMPILER_VERSION_MAJOR, PLATFORM_COMPILER_VERSION_MINOR);
	emulator_log(true, LOG_SEVERITY_INFO, "Project github: https://github.com/RandomComp/Emulator_OS");
	emulator_log(true, LOG_SEVERITY_INFO, "Running on %s", buf);

	#ifndef EMULATOR_SDL_USING
	#ifdef IS_UNIX
	orig_termios = switch_to_raw();
	#endif

	change_title("OS Emulator (non-gui mode) (Ctrl+C to exit)");

	#ifdef IS_UNIX
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
	#endif

	setbuf(stdout, NULL);
	
	ssize_t columns = 80, rows = 25;

	printf(invisible_cursor);
	#endif

	// get_terminal_size(&columns, &rows);

	emulator_log(true, LOG_SEVERITY_INFO, "Kernel \"%s\" loading...", so_name);

	void* os_handle = dlopen(so_name, RTLD_NOW);

	if (!os_handle) {
		emulator_log(true, LOG_SEVERITY_ERROR, "dlopen error: %s. Did you forgot to compile kernel?", dlerror());

		return 1;
	}

	void (*__emulator_init_kernel)(__init_kernel_args_t kernel_args);

	multiboot_section_t* (*__emulator_read_multiboot_secton)(void);

	void (*kmain)(uint32 magic, multiboot_info_t* multiboot);

	__emulator_init_kernel = dlsym(os_handle, "__emulator_init_kernel");
	
	if (!__emulator_init_kernel) {
		emulator_log(true, LOG_SEVERITY_ERROR, "No such function named \"__emulator_init_kernel\" in provided \"%s\". Please verify the function name.", so_name);

		dlclose(os_handle);

		return 1;
	}

	__emulator_read_multiboot_secton = dlsym(os_handle, "__emulator_read_multiboot_secton");
	
	if (!__emulator_read_multiboot_secton) {
		emulator_log(true, LOG_SEVERITY_ERROR, "No such function named \"__emulator_read_multiboot_secton\" in provided \"%s\". Please verify the function name.", so_name);

		dlclose(os_handle);

		return 1;
	}

	kmain = dlsym(os_handle, "kmain");

	if (!kmain) {
		emulator_log(true, LOG_SEVERITY_ERROR, "No such function named \"kmain\" in provided \"%s\". Please verify the function name.", so_name);

		dlclose(os_handle);

		return 1;
	}

	emulator_log(true, LOG_SEVERITY_INFO, "Kernel \"%s\" loaded!", so_name);

	emulator_log(true, LOG_SEVERITY_INFO, "Reading multiboot section from kernel \"%s\"...", so_name);

	multiboot_section_t* multiboot_section = __emulator_read_multiboot_secton();

	emulator_log(true, LOG_SEVERITY_INFO, "Multiboot section from kernel \"%s\" readed", so_name);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->magic = %u", multiboot_section->magic);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->flags = %u", multiboot_section->flags);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->checksum = %u", multiboot_section->checksum);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->unused_0 = %u", multiboot_section->unused_0);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->unused_1 = %u", multiboot_section->unused_1);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->unused_2 = %u", multiboot_section->unused_2);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->unused_3 = %u", multiboot_section->unused_3);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->unused_4 = %u", multiboot_section->unused_4);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->mode_type = %u", multiboot_section->mode_type);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->width = %u", multiboot_section->width);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->height = %u", multiboot_section->height);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->depth = %u", multiboot_section->depth);
	
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);

	uint64 init_start_time = (ts.tv_sec * 1000000) + ts.tv_nsec / 1000;

	emulator = init_emulator(
		true, 
		multiboot_section->width, multiboot_section->height, 
		multiboot_section->width, multiboot_section->height, multiboot_section->depth, true, 
		FRAMETIME_NS, HALTED_FRAMETIME_NS);

	#ifdef EMULATOR_SDL_USING
	// SDL_SetWindowSize(emulator->window, 80 * 8 * 2, 25 * 16 * 2);

	SDL_SetWindowPosition(emulator->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	#endif

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
	kernel_args.__emulator_kernel_report = report;

	__emulator_init_kernel(kernel_args);

	emulator_log(true, LOG_SEVERITY_INFO, "Kernel initialized!");

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Emulator initializing error");

		dlclose(os_handle);

		imd_exit_emulator(1);

		return 1;
	}

	timespec_get(&ts, TIME_UTC);

	uint64 emulator_init_dur = ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000)) - init_start_time;

	emulator_log(true, LOG_SEVERITY_INFO, "Initialization duration: %llu us", emulator_init_dur);

	// const c_str msg = "OS Booting (Loader message)..."; const size_t msg_len = strlen(msg);

	// const _size_t centered_column = (emulator->vga_gpu->width / 2) - (msg_len / 2);

	// const _size_t centered_row = (emulator->vga_gpu->height / 2) - 1;

	// draw_vga_text(emulator->vga_gpu, msg, 0x1F, centered_column, centered_row);

	pthread_t kmain_thread = run_emulator(emulator, kmain);

	// emulator->running = true;

	#ifdef EMULATOR_SDL_USING
	
	SDL_Event event;

	int win_x = 0, win_y = 0;

	while (emulator && emulator->running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because pressed quit button...");

				emulator->running = false; break;
			}

			if (event.type == SDL_KEYDOWN) {
				bool is_ctrl = (event.key.keysym.mod & KMOD_CTRL) || (event.key.keysym.mod & KMOD_GUI);
				bool is_shift = (event.key.keysym.mod & KMOD_SHIFT);

				// if (is_ctrl && is_shift) {
				// 	switch (event.key.keysym.sym) {
				// 		case SDLK_c:
				// 			handle_copy_selected();
				// 			break;
				// 		case SDLK_v:
				// 			handle_paste_selected();
				// 			break;
						
				// 		default:
				// 			break;
				// 	}
				// }

				// else
					handle_key_gui(event.key.keysym.scancode, false);
			}

			else if (event.type == SDL_KEYUP) {
				handle_key_gui(event.key.keysym.scancode, true);
			}

			// else if (event.type == SDL_MOUSEBUTTONDOWN) {
			// 	if (event.button.button == 1) {
			// 		SDL_GetWindowSize(emulator->window, &win_x, &win_y);

			// 		handle_mouse_button(event.motion.x, event.motion.y, win_x, win_y, false);
			// 	}
			// }

			// else if (event.type == SDL_MOUSEBUTTONUP) {
			// 	if (event.button.button == 1) {
			// 		SDL_GetWindowSize(emulator->window, &win_x, &win_y);

			// 		handle_mouse_button(event.motion.x, event.motion.y, win_x, win_y, true);
			// 	}
			// }

			// else if (event.type == SDL_MOUSEMOTION) {
			// 	SDL_GetWindowSize(emulator->window, &win_x, &win_y);

			// 	handle_mouse_move(event.motion.x, event.motion.y, win_x, win_y);
			// }
		}

		if (!emulator->is_hardware_reseting)
			emulator_update_all(emulator);

		SDL_SetRenderDrawColor(emulator->renderer, 0, 0, 0, 255);
		SDL_RenderClear(emulator->renderer);

		// TODO: обновлять экран только при необходимости

		SDL_RenderCopy(emulator->renderer, emulator->screen_texture, null, null);

		SDL_RenderPresent(emulator->renderer);

		if (!(emulator->cpu->halted) && cpu_need_to_halt) {
			set_halt(emulator->cpu);
		}
			
		itval_ns = cpu_get_itval_ns(emulator->cpu);

		SDL_Delay((uint32)(itval_ns / 1000000));
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
	
	emulator_release_tick_timer(emulator, handle_keys_cli);
	
	imd_exit_emulator(0);

	if (os_handle) dlclose(os_handle); os_handle = nullptr;

	#ifndef EMULATOR_SDL_USING
	printf(visible_cursor);
	#endif

	return 0;
}
