#ifndef _EMULATOR_OS_KERNEL_H
#define _EMULATOR_OS_KERNEL_H

#include "types.h"

#ifdef __EMULATOR__

#include "idt.h"

typedef struct __init_kernel_args_t {
	void* (*__emulator_get_ram)(void);

	size_t (*__emulator_port_in)(uint16 port);

	void (*__emulator_port_out)(uint16 port, size_t value);

	void (*__emulator_wait_halt)(void);

	void (*__emulator_sti)(void);

	void (*__emulator_cli)(void);

	uint64 (*__emulator_start_tsc)(void);

	void (*__emulator_idt_flush)(idt_ptr_t* ptr);
} __init_kernel_args_t;
#endif

void report(const byte* msg);

#endif
