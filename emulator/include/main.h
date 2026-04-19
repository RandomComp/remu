#ifndef _EMULATOR_MAIN_H
#define _EMULATOR_MAIN_H

#include "types.h"

#include "time/time.h"

typedef void (*tick_timer_handler_t)();

typedef struct tick_timer_t {
	_time_t ms;

	tick_timer_handler_t handler;
} tick_timer_t;

typedef _size_t (*handler_port_in_t)(void);

typedef void (*handler_port_out_t)(_size_t value);

_size_t port_in(uint16 port);

void port_out(uint16 port, _size_t value);

void setup_port_in(uint16 port, handler_port_in_t handler);

void setup_port_out(uint16 port, handler_port_out_t handler);

void setup_tick_timer(tick_timer_handler_t handler, _time_t ms);

void exit_emulator(int code);

#endif