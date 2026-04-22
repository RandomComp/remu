#include "cpu/emulator_cpu.h"

#include "types.h"

#include <time.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <signal.h>

#include <unistd.h>

#include "main.h"

#include "ansi.h"

#include "utils.h"

cpu_t* init_cpu(time_t frametime_ns, time_t halted_frametime_ns, void (*tick)(int)) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "CPU initialization...\n");
	
	cpu_t* cpu = malloc(sizeof(cpu_t));

	memset(cpu, 0, sizeof(cpu_t));

	cpu->halted = false;
	cpu->frametime_ns = frametime_ns;
	cpu->halted_frametime_ns = halted_frametime_ns;
	cpu->tsc_start = emulator_read_tsc();

	uint64 cpu_timer_ns = cpu_get_itval_ns(cpu);

	uint64 cpu_hz = 1000000000 / cpu_timer_ns;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Initialization CPU hardware timer (%llu hz)...\n\r", cpu_hz);

	init_timer(&cpu->hardware_timerid, cpu_timer_ns, tick);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setup CPU signals... (SIGHUP, SIGINT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE, SIGSEGV, SIGTERM, SIGSTKFLT, SIGTSTP, SIGXCPU, SIGXFSZ, SIGWINCH)\n");

	cpu_setup_signals();

	emulator_log(true, LOG_SEVERITY_VERBOSE, "CPU initialized\n");

	return cpu;
}

void set_halt(cpu_t* cpu) {
	if (!cpu || cpu->halted) return;

	emulator_log(false, LOG_SEVERITY_INFO, "CPU halted\n");
	
	cpu->halted = true;

	setup_timer(cpu->hardware_timerid, cpu->halted_frametime_ns);
}

void clear_halt(cpu_t* cpu) {
	if (!cpu || !cpu->halted) return;

	cpu->halted = false;

	emulator_log(false, LOG_SEVERITY_INFO, "CPU unhalted\n");

	setup_timer(cpu->hardware_timerid, cpu->frametime_ns);
}

uint64 cpu_get_itval_ns(cpu_t* cpu) {
	if (!cpu) return 0;
	
	return cpu->halted ? cpu->halted_frametime_ns : cpu->frametime_ns;
}

void free_cpu(cpu_t* cpu) {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "CPU deinitialization...\n");

	if (cpu) free(cpu);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "CPU deinitialized\n");
}

void release_cpu(cpu_t* cpu) {
	if (cpu) free_cpu(cpu);

	#ifdef IS_UNIX
		signal(SIGHUP, SIG_DFL);

		signal(SIGINT, SIG_DFL);

		signal(SIGILL, SIG_DFL);

		signal(SIGTRAP, SIG_DFL);

		signal(SIGABRT, SIG_DFL);

		signal(SIGBUS, SIG_DFL);

		signal(SIGFPE, SIG_DFL);

		signal(SIGSEGV, SIG_DFL);

		signal(SIGTERM, SIG_DFL);

		signal(SIGSTKFLT, SIG_DFL);

		signal(SIGTSTP, SIG_DFL);

		signal(SIGXCPU, SIG_DFL);

		signal(SIGXFSZ, SIG_DFL);

		signal(SIGWINCH, SIG_DFL);
	#endif
}

void reset_cpu(cpu_t* cpu) {
	if (!cpu) return;

	emulator_log(false, LOG_SEVERITY_INFO, "CPU reseting...\n");

	clear_halt(cpu);

	cpu->tsc_start = emulator_read_tsc();

	emulator_log(false, LOG_SEVERITY_INFO, "CPU reseted\n");
}

static void sig_hup(int sig) { // 1
	emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because terminal was closed...\n");
	
	exit_emulator(0);
}

static void sig_int(int sig) { // 2
	emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because pressed Ctrl+C...\n");
	
	exit_emulator(0);
}

static void sig_ill(int sig) { // 4
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "Tried to execute illegal instruction of processor (IllegalInstructionError, ISR 6, SIG %i), and no any handler registered for this ISR, emulator exiting...\n\r", sig);
	
	exit_emulator(0);
}

static void sig_trap(int sig) { // 5
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "While the code was executing, a breakpoint was encountered (BreakPoint, ISR 3, SIG %i), and no any handler registered for this ISR, emulator exiting...\n\r", sig);
	
	exit_emulator(0);
}

static void sig_abrt(int sig) { // 6
	emulator_log(true, LOG_SEVERITY_ERROR, "Emulator aborted!\n");
	
	exit_emulator(0);
}

static void sig_bus(int sig) { // 7
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "Tried to use memory outside of array (BreakPoint, ISR 3, SIG %i), and no any handler registered for this ISR, emulator exiting...\n\r", sig);
	
	exit_emulator(0);
}

static void sig_fpe(int sig) { // 8
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "Tried to divide any number by zero (ZeroDivisionError, ISR 0, SIG %i), and no any handler registered for this ISR, emulator exiting...\n\r", sig);
	
	exit_emulator(0);
}

static void sig_segv(int sig) { // 11
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "Tried to use memory outside of array, or an emulator error (SegmentationFaultError, ISR 5, SIG %i), and no any handler registered for this ISR, emulator exiting...\n\r", sig);
	
	exit_emulator(0);
}

static void sig_term(int sig) { // 15
	emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because got signal 15 SIGTERM...\n");
	
	exit_emulator(0);
}

static void sig_stkflt(int sig) { // 16
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "FPU Stack error (X87FPUError, ISR 16, SIG %i), and no any handler registered for this ISR, emulator exiting...\n\r", sig);
	
	exit_emulator(0);
}

static void sig_tstp(int sig) { // 20
	emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because got signal 20 SIGTSTP...\n");
	
	exit_emulator(0);
}

static void sig_xcpu(int sig) { // 24
	emulator_log(true, LOG_SEVERITY_ERROR, "An emulator error, because emulator tried to exceed his processor time (SIG %i), emulator exiting...\n\r", sig);
	
	exit_emulator(0);
}

static void sig_xfsz(int sig) { // 25
	emulator_log(true, LOG_SEVERITY_ERROR, "An emulator error, because emulator tried to exceed emulator log file (SIG %i), emulator exiting...\n\r", sig);
	
	exit_emulator(0);
}

static void sig_winch(int sig) { // 28
	// ssize_t columns, rows;

	// get_terminal_size(&columns, &rows);

	// init_screen(columns, rows);
}

void cpu_setup_signals() {
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