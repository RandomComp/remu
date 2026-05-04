#ifndef _EMULATOR_RAM_H
#define _EMULATOR_RAM_H

#include "types.h"

#include <stddef.h>

struct ram_t {
	byte* mem_ptr;

	size_t mem_size;
};

typedef struct ram_t ram_t;

ram_t* init_ram(size_t mem_size);

void reset_ram(ram_t* ram);

void free_ram(ram_t* ram);

#endif
