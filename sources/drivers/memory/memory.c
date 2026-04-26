#include "drivers/memory/memory.h"

#include "types.h"

#ifndef FREE_STANDING_MODE
#include "kernel.h"

extern __init_kernel_args_t kernel_args;
#endif

void* get_ram() {
	#ifdef FREE_STANDING_MODE
	return (void*)0;
	#else
	if (kernel_args.__emulator_get_ram)
		return kernel_args.__emulator_get_ram();
	#endif

	return nullptr;
}

uint8 in8(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint8 data = 0;

	asm volatile ("inb %%dx, %%al" :"=a" (data) : "d" (port));

	return data;
	#else
	if (kernel_args.__emulator_port_in)
		return (uint8)kernel_args.__emulator_port_in(port);
	#endif

	return 0;
}

uint16 in16(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint16 data = 0;

	asm volatile ("inw %%dx, %%ax" : "=a" (data) : "d" (port));

	return data;
	#else
	if (kernel_args.__emulator_port_in)
		return (uint16)kernel_args.__emulator_port_in(port);
	#endif

	return 0;
}

uint32 in32(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint32 data = 0;

	asm volatile ("inl %%dx, %%eax" : "=a" (data) : "d" (port));

	return data;
	#else
	if (kernel_args.__emulator_port_in)
		return (uint32)kernel_args.__emulator_port_in(port);
	#endif

	return 0;
}

void out8(uint16 port, uint8 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outb %%al, %%dx" : : "a" (data), "d" (port));
	#else
	if (kernel_args.__emulator_port_out)
		kernel_args.__emulator_port_out(port, (size_t)data);
	#endif
}

void out16(uint16 port, uint16 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outw %%ax, %%dx" : : "a" (data), "d" (port));
	#else
	if (kernel_args.__emulator_port_out)
		kernel_args.__emulator_port_out(port, (size_t)data);
	#endif
}

void out32(uint16 port, uint32 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outl %%eax, %%dx" : : "a" (data), "d" (port));
	#else
	if (kernel_args.__emulator_port_out)
		kernel_args.__emulator_port_out(port, (size_t)data);
	#endif
}
