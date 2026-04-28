#ifndef _EMULATOR_PIT_H
#define _EMULATOR_PIT_H

#include "cpu/emulator_pic.h"

#include "emulator_fwd.h"

#include "types.h"

#include <time.h>

#define PIT_INT				0x20
#define PIT_FREQ			1193182
#define PIT_DEFAULT_DIVISOR 65536

typedef struct pit_t {
	pic_t* pic;

	timer_t timer;

	uint64 counter;

	uint16 divisor;
} pit_t;

void setup_pit(emulator_t* emulator);

pit_t* init_pit(pic_t* pic);

void free_pit(pit_t* pit);

void release_pit(emulator_t* emulator, pit_t* pit);

#endif
