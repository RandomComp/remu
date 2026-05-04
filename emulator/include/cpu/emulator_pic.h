#ifndef _EMULATOR_PIC_H
#define _EMULATOR_PIC_H

#include "types.h"

typedef void (*isr_handler_t)(void);

typedef struct PACKED idt_entry_t {
	uint32 base_low;

	uint16 sel;

	uint8 always0;

	uint8 flags;

	uint32 base_high;
} idt_entry_t;

typedef struct PACKED idt_ptr_t {
	uint16 limit;

	uint64 base;
} idt_ptr_t;

#define HANDLER_QUEUE_SIZE_STEP (4)

#define HANDLER_QUEUE_SIZE (64)

typedef struct pic_t {
	isr_handler_t handler_queue[HANDLER_QUEUE_SIZE];
	// isr_handler_t* handler_queue;
	// _size_t queue_size;
	_size_t handler_pos;
	
	bool lock; bool allow_ints;
} pic_t;

#include "cpu/emulator_cpu.h"

pic_t* init_emulator_pic(cpu_t* cpu);

void set_sti_pic(pic_t* pic);
void set_cli_pic(pic_t* pic);

void exec_all_emulator_ints(pic_t* pic);

void call_emulator_int(pic_t* pic, byte _int);

void idt_flush_emulator(idt_ptr_t* ptr);

void free_emulator_pic(pic_t* pic);

#endif
