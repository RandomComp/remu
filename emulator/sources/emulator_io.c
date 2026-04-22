#include "emulator_io.h"

#include "types.h"

#include "main.h"

#include <string.h>

static handler_port_in_t table_port_in[65536] = { 0 };

static handler_port_out_t table_port_out[65536] = { 0 };

_size_t port_in(uint16 port) {
	handler_port_in_t result = table_port_in[port];

	if (result)
		return result();
	else
		emulator_log(false, LOG_SEVERITY_WARNING, "Tried read free port 0x%hx\n", port);

	return 0;
}

void port_out(uint16 port, _size_t value) {
	handler_port_out_t result = table_port_out[port];

	if (result)
		result(value);
	else
		emulator_log(false, LOG_SEVERITY_WARNING, "Tried write 0x%llx to free port 0x%hx\n", value, port);
}

void emulator_setup_port_in(uint16 port, handler_port_in_t handler) {
	table_port_in[port] = handler;
}

void emulator_setup_port_out(uint16 port, handler_port_out_t handler) {
	table_port_out[port] = handler;
}

void emulator_release_port_in(uint16 port) {
	table_port_in[port] = nullptr;
}

void emulator_release_port_out(uint16 port) {
	table_port_out[port] = nullptr;
}