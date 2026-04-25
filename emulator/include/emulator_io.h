#ifndef _EMULATOR_IO_H
#define _EMULATOR_IO_H

#include "types.h"

typedef _size_t (*handler_port_in_t)(void);

typedef void (*handler_port_out_t)(_size_t value);

_size_t port_in(uint16 port);

void port_out(uint16 port, _size_t value);

void emulator_setup_port_in(uint16 port, handler_port_in_t handler);

void emulator_setup_port_out(uint16 port, handler_port_out_t handler);

void emulator_release_port_in(uint16 port);

void emulator_release_port_out(uint16 port);

#endif
