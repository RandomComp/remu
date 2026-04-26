#include "cpu/emulator_cpu.h"

#include "emulator_logger.h"

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

#include "cpu/emulator_pic.h"

static void sig_hup(int UNUSED sig);
static void sig_int(int UNUSED sig);
static void sig_ill(int UNUSED sig);
static void sig_trap(int UNUSED sig);
static void sig_abrt(int UNUSED sig);
static void sig_bus(int UNUSED sig);
static void sig_fpe(int UNUSED sig);
static void sig_segv(int UNUSED sig);
static void sig_term(int UNUSED sig);
static void sig_stkflt(int UNUSED sig);
static void sig_tstp(int UNUSED sig);
static void sig_xcpu(int UNUSED sig);
static void sig_xfsz(int UNUSED sig);
static void sig_winch(int UNUSED sig);

const int cpu_signals[] = {
	SIGHUP, SIGINT, SIGILL, SIGTRAP, SIGABRT, 
	SIGBUS, SIGFPE, SIGSEGV, SIGTERM, SIGSTKFLT, 
	SIGTSTP, SIGXCPU, SIGXFSZ, SIGWINCH
};

signal_handler_t cpu_signals_handler[14];

const size_t cpu_signals_cnt = sizeof(cpu_signals) / sizeof(cpu_signals[0]);

cpu_t* init_cpu(uint64 frametime_ns, uint64 halted_frametime_ns) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "CPU initialization...");
	
	cpu_t* cpu = malloc(sizeof(cpu_t));

	memset(cpu, 0, sizeof(cpu_t));

	cpu->pic = init_emulator_pic();
	cpu->halted = false;
	cpu->frametime_ns = frametime_ns;
	cpu->halted_frametime_ns = halted_frametime_ns;
	cpu->tsc_start = emulator_read_tsc();

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setup CPU signals... (SIGHUP, SIGINT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE, SIGSEGV, SIGTERM, SIGSTKFLT, SIGTSTP, SIGXCPU, SIGXFSZ, SIGWINCH)");
	
	cpu_setup_signals();

	emulator_log(true, LOG_SEVERITY_VERBOSE, "CPU initialized");

	return cpu;
}

void set_halt(cpu_t* cpu) {
	if (!cpu || cpu->halted) return;

	emulator_log(false, LOG_SEVERITY_INFO, "CPU halted");
	
	cpu->halted = true;
}

void clear_halt(cpu_t* cpu) {
	if (!cpu || !cpu->halted) return;

	cpu->halted = false;

	emulator_log(false, LOG_SEVERITY_INFO, "CPU unhalted");
}

uint64 cpu_get_itval_ns(cpu_t* cpu) {
	if (!cpu) return 0;

	return cpu->halted ? cpu->halted_frametime_ns : cpu->frametime_ns;
}

void free_cpu(cpu_t* cpu) {
	if (!cpu) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "CPU deinitialization...");

	free_emulator_pic(cpu->pic);

	free(cpu);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "CPU deinitialized");
}

void release_cpu(cpu_t* cpu) {
	if (cpu) free_cpu(cpu);

	#ifdef IS_UNIX
		for (size_t i = 0; i < cpu_signals_cnt; i++) {
			signal(cpu_signals[i], SIG_DFL);
		}
	#endif
}

void reset_cpu(cpu_t* cpu) {
	if (!cpu) return;

	emulator_log(false, LOG_SEVERITY_INFO, "CPU reseting...");

	clear_halt(cpu);

	cpu->tsc_start = emulator_read_tsc();

	// reset_emulator_pic();

	emulator_log(false, LOG_SEVERITY_INFO, "CPU reseted");
}

static void sig_hup(int UNUSED sig) { // 1
	emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because terminal was closed...");
	
	imd_exit_emulator(0);

	exit(0);
}

static void sig_int(int UNUSED sig) { // 2
	emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because pressed Ctrl+C...");
	
	imd_exit_emulator(0);

	exit(0);
}

static void sig_ill(int UNUSED sig) { // 4
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "Tried to execute illegal instruction of processor (IllegalInstructionError, ISR 6, SIG %i), and no any handler registered for this ISR, emulator exiting...", sig);

	exit(1);
}

static void sig_trap(int UNUSED sig) { // 5
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "While the code was executing, a breakpoint was encountered (BreakPoint, ISR 3, SIG %i), and no any handler registered for this ISR, emulator exiting...", sig);
	
	exit(1);
}

extern logger_t* logger;

static void sig_abrt(int UNUSED sig) { // 6
	emulator_log(true, LOG_SEVERITY_ERROR, "Emulator aborted!");

	free_emulator_logger(logger);
	
	exit(1);
}

static void sig_bus(int UNUSED sig) { // 7
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "Tried to use memory outside of array (BreakPoint, ISR 3, SIG %i), and no any handler registered for this ISR, emulator exiting...", sig);
	
	imd_exit_emulator(1);

	exit(1);
}

static void sig_fpe(int UNUSED sig) { // 8
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "Tried to divide any number by zero (ZeroDivisionError, ISR 0, SIG %i), and no any handler registered for this ISR, emulator exiting...", sig);
	
	imd_exit_emulator(1);

	exit(1);
}

static void sig_segv(int UNUSED sig) { // 11
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "Tried to use memory outside of array, or an emulator error (SegmentationFaultError, ISR 5, SIG %i), and no any handler registered for this ISR, emulator exiting...", sig);
	
	imd_exit_emulator(1);

	exit(1);
}

static void sig_term(int UNUSED sig) { // 15
	emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because got signal 15 SIGTERM...");
	
	imd_exit_emulator(0);

	exit(0);
}

static void sig_stkflt(int UNUSED sig) { // 16
	// TODO: вызывать тут обработчик из таблицы прерываний

	emulator_log(true, LOG_SEVERITY_ERROR, "FPU Stack error (X87FPUError, ISR 16, SIG %i), and no any handler registered for this ISR, emulator exiting...", sig);
	
	imd_exit_emulator(1);

	exit(1);
}

static void sig_tstp(int UNUSED sig) { // 20
	emulator_log(true, LOG_SEVERITY_INFO, "Exiting emulator because got signal 20 SIGTSTP...");
	
	imd_exit_emulator(0);

	exit(0);
}

static void sig_xcpu(int UNUSED sig) { // 24
	emulator_log(true, LOG_SEVERITY_ERROR, "An emulator error, because emulator tried to exceed his processor time (SIG %i), emulator exiting...", sig);
	
	imd_exit_emulator(1);

	exit(1);
}

static void sig_xfsz(int UNUSED sig) { // 25
	emulator_log(true, LOG_SEVERITY_ERROR, "An emulator error, because emulator tried to exceed emulator log file (SIG %i), emulator exiting...", sig);
	
	imd_exit_emulator(1);

	exit(1);
}

static void sig_winch(int UNUSED sig) { // 28
	// ssize_t columns, rows;

	// get_terminal_size(&columns, &rows);

	// init_screen(columns, rows);
}

void cpu_setup_signals() {
	#ifdef IS_UNIX
		cpu_signals_handler[0] = sig_hup;
		cpu_signals_handler[1] = sig_int;
		cpu_signals_handler[2] = sig_ill;
		cpu_signals_handler[3] = sig_trap;
		cpu_signals_handler[4] = sig_abrt;
		cpu_signals_handler[5] = sig_bus;
		cpu_signals_handler[6] = sig_fpe;
		cpu_signals_handler[7] = sig_segv;
		cpu_signals_handler[8] = sig_term;
		cpu_signals_handler[9] = sig_stkflt;
		cpu_signals_handler[10] = sig_tstp;
		cpu_signals_handler[11] = sig_xcpu;
		cpu_signals_handler[12] = sig_xfsz;
		cpu_signals_handler[13] = sig_winch;
	
		for (size_t i = 0; i < cpu_signals_cnt; i++) {
			signal(cpu_signals[i], cpu_signals_handler[i]);
		}
	#endif
}
