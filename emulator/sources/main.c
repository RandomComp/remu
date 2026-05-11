#include "main.h"

// ╨Ü╨╛╨┤ ╤ì╤é╨╛╨│╨╛ ╤ì╨╝╤â╨╗╤Å╤é╨╛╤Ç╨░ 0xEA32

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

// TODO: ╨í╨┤╨╡╨╗╨░╤é╤î ╨┤╨▓╨╛╨╣╨╜╨╛╨╣ ╨▒╤â╤ä╨╡╤Ç ╨▓╨╕╨┤╨╡╨╛╨┐╨░╨╝╤Å╤é╨╕

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

	if (!emulator) return;

	stop_emulator(emulator);
	
	emulator->running = false;
}

void imd_exit_emulator(int code) {
	if (emulator) {
		exit_emulator(0);

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

#ifdef IS_WIN
#warning "This emulator now is not fully supported on windows, use it for your own risk"
#endif

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

	const byte* so_name = argv[1];

	atexit(on_emulator_exit);

	#ifdef EMULATOR_SDL_USING
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize SDL video subsystem: %s", SDL_GetError());

		return 1;
	}

	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
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
	
	kernel_t* kernel = emulator_load_kernel(so_name);

	if (!kernel) return 1;

	emulator_log(true, LOG_SEVERITY_INFO, "Reading multiboot section from kernel \"%s\"...", so_name);

	multiboot_section_t* multiboot_section = kernel->__emulator_read_multiboot_secton();

	emulator_log(true, LOG_SEVERITY_INFO, "Multiboot section from kernel \"%s\" readed", so_name);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->magic = %x", multiboot_section->magic);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->flags = %8b", multiboot_section->flags);
	emulator_log(true, LOG_SEVERITY_VERBOSE, "multiboot_section->checksum = %x", multiboot_section->checksum);
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

	if (multiboot_section->mode_type == 0) {
		emulator = init_emulator(
			true, 
			multiboot_section->width, multiboot_section->height, 
			multiboot_section->width, multiboot_section->height, multiboot_section->depth,
			true, 
			FRAMETIME_NS, HALTED_FRAMETIME_NS
		);
	}
	
	else {
		emulator = init_emulator(
			true, 
			multiboot_section->width * 8, multiboot_section->height * 16, 
			multiboot_section->width, multiboot_section->height, multiboot_section->depth,
			false, 
			FRAMETIME_NS, HALTED_FRAMETIME_NS
		);
	}

	#ifdef EMULATOR_SDL_USING
	if (multiboot_section->mode_type != 0) {
		SDL_SetWindowSize(emulator->window, multiboot_section->width * 8 * 2, multiboot_section->height * 16 * 2);
	}

	SDL_SetWindowPosition(emulator->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	#endif

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Emulator initializing error");

		imd_exit_emulator(1);

		return 1;
	}

	timespec_get(&ts, TIME_UTC);

	uint64 emulator_init_dur = ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000)) - init_start_time;

	emulator_log(true, LOG_SEVERITY_INFO, "Initialization duration: %llu us", emulator_init_dur);

	if (multiboot_section->mode_type != 0) {
		const byte* msg = "OS Booting (Loader message)..."; const size_t msg_len = strlen(msg);

		const _size_t centered_column = (emulator->vga_gpu->width / 2) - (msg_len / 2);

		const _size_t centered_row = (emulator->vga_gpu->height / 2) - 1;

		draw_vga_text(emulator->vga_gpu, msg, 0x1F, centered_column, centered_row);
	}

	emulator->kernel = kernel;

	pthread_t kmain_thread = 0;

	emulator->running = true;

	emulator_forced_update_all_timers(emulator);

	while (emulator->running || emulator->is_hardware_reseting) {
		kmain_thread = run_emulator(emulator);

		main_loop(emulator, multiboot_section);
	}
	
	imd_exit_emulator(0);

	#ifndef EMULATOR_SDL_USING
	printf(visible_cursor);
	#endif

	return 0;
}
