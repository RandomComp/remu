#include "time/emulator_pit.h"

#include "cpu/emulator_pic.h"

#include "emulator.h"

#include "emulator_logger.h"

#include "utils.h"

#include <malloc.h>

#include <string.h>

static pit_t* cur = nullptr;

static void gen_tick(void) {
	if (cur && cur->pic)
		call_emulator_int(cur->pic, PIT_INT);
}

static uint64 get_ms_from_divisor(uint16 _divisor) {
	uint64 divisor = (uint64)((_divisor != 0) ? _divisor : PIT_DEFAULT_DIVISOR);

	return (uint64)(PIT_FREQ * 100) / (divisor * 100);
}

void setup_pit(emulator_t* emulator) {
	// TODO: ╨ƒ╨╡╤Ç╨╡╨╜╨╡╤ü╤é╨╕ ╤é╨░╨╣╨╝╨╡╤Ç ╨╜╨░ API Linux

	emulator_setup_tick_timer(emulator, gen_tick, get_ms_from_divisor(0));
}

pit_t* init_pit(pic_t* pic) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "PIT initializing...");

	if (!pic) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize PIT: no PIC instance provided");

		return nullptr;
	}

	pit_t* pit = malloc(sizeof(pit_t));

	memset(pit, 0, sizeof(pit_t));

	pit->pic = pic;

	pit->divisor = 0;

	cur = pit;
	
	emulator_log(true, LOG_SEVERITY_VERBOSE, "PIT initialized!");

	return pit;
}

void free_pit(pit_t* pit) {
	if (pit) {
		timer_delete(pit->timer);

		if (cur == pit) cur = nullptr;

		free(pit); pit = nullptr;
	}
}

void release_pit(emulator_t* emulator, pit_t* pit) {
	emulator_release_tick_timer(emulator, gen_tick);

	if (pit) free_pit(pit);

	if (cur == pit) cur = nullptr;
}
