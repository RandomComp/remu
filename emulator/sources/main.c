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

#include "math/math.h"

#include "kernel.h"

#include "utils.h"

#include "emulator.h"

#include "emulator_logger.h"

#include <pthread.h>

#include <signal.h>

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#endif

#ifdef IS_UNIX
struct termios* orig_termios;
#endif

emulator_t* emulator = nullptr;

// TODO: Сделать двойной буфер видеопамяти

// TODO: Сделать таблицу прерываний и их настройку

static void on_emulator_exit() {
	if (orig_termios) {
		switch_to_default(orig_termios);

		free(orig_termios);
	}

	imd_exit_emulator(0);
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

		pthread_cancel(emulator->kmain_thread);

		free_emulator(emulator);

		free_emulator_logger();

		IMG_Quit();
		SDL_Quit();

		emulator = nullptr;
	}
}

static void timer_handler(int sig) {
	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot handle the timer: no emulator instance provided");

		imd_exit_emulator(1);

		return;
	}

	emulator_update_all(emulator);
}

// extern void printf(const char* format, int len);

// extern void exit123(int);

int main() {
	#ifdef IS_WIN
	printf("This emulator now is not fully supported on windows, use it for your own risk\n\r");
	#endif

	atexit(on_emulator_exit);

	init_emulator_logger();

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

	change_title("OS Emulator (Ctrl+C to exit)");

	#ifdef IS_UNIX
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
	#endif

	setbuf(stdout, NULL);
	
	ssize_t columns = 80, rows = 25;

	printf(invisible_cursor);
	#endif

	// get_terminal_size(&columns, &rows);
	
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);

	uint64 init_start_time = (ts.tv_sec * 1000000) + ts.tv_nsec / 1000;

	emulator = init_emulator(true, 640, 400, FRAMETIME_NS, FRAMETIME_NS);

	if (!emulator) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Emulator initializing error");

		imd_exit_emulator(1);

		return 1;
	}

	emulator_logger_set_emulator(emulator);

	timespec_get(&ts, TIME_UTC);

	uint64 emulator_init_dur = ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000)) - init_start_time;

	emulator_log(true, LOG_SEVERITY_INFO, "Initialization duration: %llu us", emulator_init_dur);

	pthread_t kmain_thread = run_emulator(emulator, kmain);

	#ifdef EMULATOR_SDL_USING
	
	SDL_Event event;

	while (emulator && emulator->running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because pressed quit button...");

				emulator->running = false; break;
			}
		}

		if (!emulator->is_hardware_reseting)
			emulator_update_all(emulator);

		SDL_SetRenderDrawColor(emulator->renderer, 0, 0, 0, 255);
		SDL_RenderClear(emulator->renderer);

		SDL_RenderCopy(emulator->renderer, emulator->screen_texture, null, null);

		SDL_RenderPresent(emulator->renderer);

		usleep(cpu_get_itval_ns(emulator->cpu) / 1000);
	}
	#else
	while (emulator->running) {
		if (!emulator->is_hardware_reseting)
			emulator_update_all(emulator);

		usleep(cpu_get_itval_ns(emulator->cpu) / 1000);
	}
	#endif
	
	imd_exit_emulator(0);

	#ifndef EMULATOR_SDL_USING
	printf(visible_cursor);
	#endif

	return 0;
}
