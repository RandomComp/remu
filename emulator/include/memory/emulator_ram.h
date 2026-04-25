#ifndef _EMULATOR_RAM_H
#define _EMULATOR_RAM_H

#include "types.h"

typedef struct ram_t {
	byte* mem_ptr;

	_size_t mem_size;
} ram_t;

ram_t* init_ram(_size_t mem_size);

void reset_ram(ram_t* ram);

void free_ram(ram_t* ram);

#endif
