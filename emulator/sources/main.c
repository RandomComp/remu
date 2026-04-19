#include "main.h"

#include "types.h"

#include <stdio.h>

#include <stdlib.h>

#ifdef IS_UNIX
#include <signal.h>

#include <unistd.h>

#include <fcntl.h>

#include <sys/ioctl.h>
#endif

#include <time.h>

#include <string.h>

#include "ansi.h"

#include "kernel.h"

#include "math/math.h"

#include "drivers/time/tsc.h"

#include "cpu/emulator_cpu.h"

#include "vga/emulator_vga.h"

#include "time/emulator_cmos.h"

#include "utils.h"

#define INPUT_BUFFER_MAX_SIZE 128

size_t 	screen_width = 0, 
		screen_height = 0;

#define frametime_ns 1000 * 1000 * 20

#define halted_frametime_ns 1000 * 1000 * 200

byte input_buffer[INPUT_BUFFER_MAX_SIZE] = { 0 };

_size_t input_buffer_offset = 0;

cpu_t* cpu = nullptr;

vga_text_screen_t* vga = nullptr;

cmos_t* cmos = nullptr;

struct termios* orig_termios;

// TODO: Сделать двойной буфер видеопамяти

// TODO: Сделать таблицу прерываний и их настройку

static void handle_keys();

static void update();

static void timer_handler(int sig);

static handler_port_in_t table_port_in[65536] = { 0 };

static handler_port_out_t table_port_out[65536] = { 0 };

static tick_timer_t* tick_timers = nullptr; static size_t tick_timers_cnt = 0;

static time_t ticks = 0;

static size_t input_buffer_pos = 0;

static bool keyboard_reading_locked = false;

static _size_t keyboard_data_port_handler() {
	//while (keyboard_reading_locked);

	keyboard_reading_locked = true;

	_size_t result = input_buffer[input_buffer_pos];

	if (result) {
		input_buffer_pos = (input_buffer_pos + 1) % INPUT_BUFFER_MAX_SIZE;
	}

	keyboard_reading_locked = false;

	return result;
}

static _size_t keyboard_command_port_handler() {
	return 1;
}

static void init_default_table_port_in() {
	table_port_in[0x60] = &keyboard_data_port_handler;

	table_port_in[0x64] = &keyboard_command_port_handler;
}

static void old_qemu_bochs_shutdown(_size_t value) {
	if (value != 0x2000) return;

	exit_emulator(0);
}

static void modern_qemu_power_command(_size_t value) {
	// if (value != 0x2000) return;

	exit_emulator(0);
}

static void virtual_box_acpi_pm1a_cnt_command(_size_t value) {
	if (value != 0x3400) return;

	exit_emulator(0);
}

static void cloud_hypervisor_power_command(_size_t value) {
	if (value != 0x34) return;

	exit_emulator(0);
}

static void init_default_table_port_out() {
	table_port_out[0xB004] = &old_qemu_bochs_shutdown;

	table_port_out[0x604] = &modern_qemu_power_command;

	table_port_out[0x4004] = &virtual_box_acpi_pm1a_cnt_command;

	table_port_out[0x600] = &cloud_hypervisor_power_command;
}

_size_t port_in(uint16 port) {
	handler_port_in_t result = table_port_in[port];

	if (result)
		return result();

	return 0;
}

void port_out(uint16 port, _size_t value) {
	handler_port_out_t result = table_port_out[port];

	if (result)
		result(value);
}

void setup_port_in(uint16 port, handler_port_in_t handler) {
	table_port_in[port] = handler;
}

void setup_port_out(uint16 port, handler_port_out_t handler) {
	table_port_out[port] = handler;
}

void setup_tick_timer(tick_timer_handler_t handler, _time_t ms) {
	tick_timers = realloc(tick_timers, (tick_timers_cnt + 1) * sizeof(tick_timer_t));

	tick_timers[tick_timers_cnt] = (tick_timer_t){ .handler = handler, .ms = ms };

	tick_timers_cnt += 1;
}

static void handle_keys() {
	// while (keyboard_reading_locked);

	keyboard_reading_locked = true;

	char c = '\0';

	ssize_t read_num = read(STDIN_FILENO, &c, 1);

	if (read_num == 0 || c == 0x3) {
		exit_emulator(0);

		return;
	}
	
	if (c != '\0') {
		clear_halt(cpu);

		input_buffer[input_buffer_offset] = c;

		input_buffer_offset = (input_buffer_offset + 1) % INPUT_BUFFER_MAX_SIZE;
	}
	
	input_buffer[(input_buffer_offset + 1) % INPUT_BUFFER_MAX_SIZE] = 0;

	keyboard_reading_locked = false;
}

static void update() {
	draw_vga_text_screen(vga);

	handle_keys();
}

static void timer_handler(int sig) {
	if (tick_timers) {
		for (size_t i = 0; i < tick_timers_cnt; i++) {
			tick_timer_t* tick_timer = &tick_timers[i];

			if (tick_timer && (ticks % tick_timer->ms) == 0) {
				tick_timer->handler();
			}
		}
	}

	update();

	ticks += (time_t)((uint64)get_itval_ns(cpu) / 1000000);
}

void exit_emulator(int code) {
	if (cpu) {
		clear_halt(cpu);

		free_cpu(cpu); cpu = null;
	}

	if (vga) {
		free_vga_text_screen(vga); vga = null;
	}

	if (cmos) {
		free_cmos(cmos); cmos = null;
	}

	printf(visible_cursor);

	// fflush(stdout);

	exit(code);
}

static void on_emulator_exit() {
	switch_to_default(orig_termios);

	free(orig_termios);
}

// extern void printf(const char* format, int len);

// extern void exit123(int);

int main() {
	#ifdef IS_WIN
	printf("This program now is unsupported on windows, try again on UNIX-like OS (macOS, Linux or similar)");
	#endif

	// printf("TSC Calibration...\n\r");

	// uint64 dur = 0, second = 0;

	// while (second <= 20) {
	// 	uint64 start = read_tsc();

	// 	sleep(1);

	// 	dur = (dur + read_tsc() - start) / 2;

	// 	printf("dur: %lu counts. %lu s/20 s\n\r", dur, second);

	// 	second += 1;
	// }

	// printf("Result: %lu counts/s\n\r", dur);

	// exit(0);

	// printf("Hello!\n", 7);

	// exit123(0);

	change_title("OS Emulator (Ctrl+C to exit)");

	orig_termios = switch_to_raw();

	atexit(on_emulator_exit);

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	setbuf(stdout, NULL);
	
	ssize_t columns = 80, rows = 25;

	// get_terminal_size(&columns, &rows);

	cpu = init_cpu(&timer_handler);

	vga = init_vga_text_screen(columns, rows);

	cmos = init_cmos();

	setup_signals();

	init_default_table_port_in();

	init_default_table_port_out();

	printf(invisible_cursor);

	kmain(0x2BADB002, nullptr);

	update();
	
	exit_emulator(0);

	return 0;
}