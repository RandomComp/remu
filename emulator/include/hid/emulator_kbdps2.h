#ifndef _EMULATOR_KBDPS2_H
#define _EMULATOR_KBDPS2_H

#include "types.h"

#define INPUT_BUFFER_MAX_SIZE 128

typedef struct kbdps2_t {
	byte input_buffer[INPUT_BUFFER_MAX_SIZE];

	_size_t input_buffer_index;
} kbdps2_t;

kbdps2_t* init_kbdps2();

void free_kbdps2(kbdps2_t* kbdps2);

void release_all_kbdps2();

#endif