#include "cpu/emulator_cpu.h"

#include "types.h"

#include <time.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <signal.h>

#include <unistd.h>

#include "drivers/time/tsc.h"

#include "main.h"

#include "ansi.h"

#include "utils.h"

cpu_t* init_cpu(void (*tick)(int)) {
	cpu_t* cpu = malloc(sizeof(cpu_t));

	memset(cpu, 0, sizeof(cpu_t));

	cpu->halted = false;
	cpu->frametime_ns = 1000 * 1000 * 20;
	cpu->halted_frametime_ns = 1000 * 1000 * 100;
	cpu->tsc_start = read_tsc();

	init_timer(&cpu->hardware_timerid, cpu->frametime_ns, tick);

	return cpu;
}

void set_halt(cpu_t* cpu) {
	if (!cpu || cpu->halted) return;

	cpu->halted = true;

	setup_timer(cpu->hardware_timerid, cpu->halted_frametime_ns);
}

void clear_halt(cpu_t* cpu) {
	if (!cpu || !cpu->halted) return;

	cpu->halted = false;

	setup_timer(cpu->hardware_timerid, cpu->frametime_ns);
}

uint64 get_itval_ns(cpu_t* cpu) {
	if (!cpu) return 0;
	
	return cpu->halted ? cpu->halted_frametime_ns : cpu->frametime_ns;
}

void free_cpu(cpu_t* cpu) {
	if (!cpu) return;

	free(cpu);
}

static void sig_hup(int sig) { // 1
	exit_emulator(0);
}

static void sig_int(int sig) { // 2
	exit_emulator(0);
}

static void sig_ill(int sig) { // 4
	// TODO: вызывать тут обработчик из таблицы прерываний

	printf("Tried to execute illegal instruction of processor (IllegalInstructionError, ISR 6), and no any handler registered for this ISR, emulator exiting...\n\r");
	
	exit_emulator(0);
}

static void sig_trap(int sig) { // 5
	// TODO: вызывать тут обработчик из таблицы прерываний

	printf("While the code was executing, a breakpoint was encountered (BreakPoint, ISR 3), and no any handler registered for this ISR, emulator exiting...\n\r");
	
	exit_emulator(0);
}

static void sig_abrt(int sig) { // 6
	printf("Emulator aborted!\n\r");
	
	exit_emulator(0);
}

static void sig_bus(int sig) { // 7
	// TODO: вызывать тут обработчик из таблицы прерываний

	printf("Tried to use memory outside of array (BreakPoint, ISR 3), and no any handler registered for this ISR, emulator exiting...\n\r");
	
	exit_emulator(0);
}

static void sig_fpe(int sig) { // 8
	// TODO: вызывать тут обработчик из таблицы прерываний

	printf("Tried to divide any number by zero (ZeroDivisionError, ISR 0), and no any handler registered for this ISR, emulator exiting...\n\r");
	
	exit_emulator(0);
}

static void sig_segv(int sig) { // 11
	// TODO: вызывать тут обработчик из таблицы прерываний

	printf("Tried to use memory outside of array, or an emulator error (SegmentationFaultError, ISR 5), and no any handler registered for this ISR, emulator exiting...\n\r");
	
	exit_emulator(0);
}

static void sig_term(int sig) { // 15
	exit_emulator(0);
}


static void sig_stkflt(int sig) { // 16
	// TODO: вызывать тут обработчик из таблицы прерываний

	printf("FPU Stack error (X87FPUError, ISR 16), and no any handler registered for this ISR, emulator exiting...\n\r");
	
	exit_emulator(0);
}

static void sig_tstp(int sig) { // 20
	exit_emulator(0);
}

static void sig_xcpu(int sig) { // 24
	printf("An emulator error, because emulator tried to exceed his processor time, emulator exiting...\n\r");
	
	exit_emulator(0);
}

static void sig_xfsz(int sig) { // 25
	printf("An emulator error, because emulator tried to exceed emulator log file, emulator exiting...\n\r");
	
	exit_emulator(0);
}

static void sig_winch(int sig) { // 28
	// ssize_t columns, rows;

	// get_terminal_size(&columns, &rows);

	// init_screen(columns, rows);
}

void setup_signals() {
	#ifdef IS_UNIX
		signal(SIGHUP, &sig_hup);

		signal(SIGINT, &sig_int);

		signal(SIGILL, &sig_ill);

		signal(SIGTRAP, &sig_trap);

		signal(SIGABRT, &sig_abrt);

		signal(SIGBUS, &sig_bus);

		signal(SIGFPE, &sig_fpe);

		signal(SIGSEGV, &sig_segv);

		signal(SIGTERM, &sig_term);

		signal(SIGSTKFLT, &sig_stkflt);

		signal(SIGTSTP, &sig_tstp);

		signal(SIGXCPU, &sig_xcpu);

		signal(SIGXFSZ, &sig_xfsz);

		signal(SIGWINCH, &sig_winch);
	#endif
}