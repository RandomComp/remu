#ifndef _EMULATOR_IDT_H
#define _EMULATOR_IDT_H

#include "types.h"

typedef void (*isr_handler_t)(void);

typedef struct PACKED idt_entry_t {
	#ifdef FREE_STANDING_MODE
	uint16 base_low;
	#else
	uint32 base_low;
	#endif

	uint16 sel;

	uint8 always0;

	uint8 flags;

	#ifdef FREE_STANDING_MODE
	uint16 base_high;
	#else
	uint32 base_high;
	#endif
} idt_entry_t;

typedef struct PACKED idt_ptr_t {
	uint16 limit;

	#ifdef FREE_STANDING_MODE
	uint32 base;
	#else
	uint64 base;
	#endif
} idt_ptr_t;

typedef struct pic_t {
	isr_handler_t handler_queue[16]; _size_t handler_pos;
	
	bool lock;
} pic_t;

pic_t* init_emulator_pic();

void exec_all_emulator_ints(pic_t* pic);

void call_emulator_int(pic_t* pic, _size_t _int);

void idt_flush_emulator(idt_ptr_t* ptr);

void free_emulator_pic(pic_t* pic);

#endif
