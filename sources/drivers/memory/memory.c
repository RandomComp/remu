#include "drivers/memory/memory.h"

#include "types.h"

#ifndef FREE_STANDING_MODE
void* (*__emulator_get_ram)(void);

size_t (*__emulator_port_in)(uint16 port);

void (*__emulator_port_out)(uint16 port, size_t value);

void (*__emulator_wait_halt)(void);

void __emulator_init_os_kernel(
	void* (*__get_ram)(void), 
	size_t (*__port_in)(uint16 port), 
	void (*__port_out)(uint16 port, size_t value), 
	void (*__wait_halt)(void)) {
	__emulator_get_ram = __get_ram;

	__emulator_port_in = __port_in;

	__emulator_port_out = __port_out;

	__emulator_wait_halt = __wait_halt;
}
#endif

void* get_ram() {
	#ifdef FREE_STANDING_MODE
	return (void*)0;
	#else
	if (__emulator_get_ram)
		return __emulator_get_ram();
	#endif

	return nullptr;
}

uint8 in8(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint8 data = 0;

	asm volatile ("inb %%dx, %%al" :"=a" (data) : "d" (port));

	return data;
	#else
	if (__emulator_port_in)
		return (uint8)__emulator_port_in(port);
	#endif

	return 0;
}

uint16 in16(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint16 data = 0;

	asm volatile ("inw %%dx, %%ax" : "=a" (data) : "d" (port));

	return data;
	#else
	if (__emulator_port_in)
		return (uint16)__emulator_port_in(port);
	#endif

	return 0;
}

uint32 in32(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint32 data = 0;

	asm volatile ("inl %%dx, %%eax" : "=a" (data) : "d" (port));

	return data;
	#else
	if (__emulator_port_in)
		return (uint32)__emulator_port_in(port);
	#endif

	return 0;
}

void out8(uint16 port, uint8 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outb %%al, %%dx" : : "a" (data), "d" (port));
	#else
	if (__emulator_port_out)
		__emulator_port_out(port, (size_t)data);
	#endif
}

void out16(uint16 port, uint16 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outw %%ax, %%dx" : : "a" (data), "d" (port));
	#else
	if (__emulator_port_out)
		__emulator_port_out(port, (size_t)data);
	#endif
}

void out32(uint16 port, uint32 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outl %%eax, %%dx" : : "a" (data), "d" (port));
	#else
	if (__emulator_port_out)
		__emulator_port_out(port, (size_t)data);
	#endif
}
