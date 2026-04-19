#include "drivers/memory/memory.h"

#include "types.h"

#ifndef FREE_STANDING_MODE
#include "main.h"

#include "vga/emulator_vga.h"

extern vga_text_screen_t* vga;
#endif

void* getMemoryOffset(PredefinedMemoryType PMTType) {
	#ifdef FREE_STANDING_MODE
	void* memoryOffsets[] = {
		[PMT_RAM_BASIC] =       (void*)RMA_RAM_BASIC,
		[PMT_TEXT_MEM_80x25] =  (void*)RMA_TEXT_MEM_80x25
	};

	return memoryOffsets[PMTType];
	#else
	if (PMTType == PMT_TEXT_MEM_80x25) return vga->vidmem;

	return nullptr;
	#endif
}

uint8 in8(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint8 data = 0;

	asm volatile ("inb %%dx, %%al" :"=a" (data) : "d" (port));

	return data;
	#else
	return (uint8)port_in(port);
	#endif

	return 0;
}

uint16 in16(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint16 data = 0;

	asm volatile ("inw %%dx, %%ax" : "=a" (data) : "d" (port));

	return data;
	#else
	return (uint16)port_in(port);
	#endif

	return 0;
}

uint32 in32(uint16 port) {
	#ifdef FREE_STANDING_MODE
	uint32 data = 0;

	asm volatile ("inl %%dx, %%eax" : "=a" (data) : "d" (port));

	return data;
	#else
	return (uint32)port_in(port);
	#endif

	return 0;
}

void out8(uint16 port, uint8 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outb %%al, %%dx" : : "a" (data), "d" (port));
	#else
	port_out(port, (_size_t)data);
	#endif
}

void out16(uint16 port, uint16 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outw %%ax, %%dx" : : "a" (data), "d" (port));
	#else
	port_out(port, (_size_t)data);
	#endif
}

void out32(uint16 port, uint32 data) {
	#ifdef FREE_STANDING_MODE
	asm volatile ("outl %%eax, %%dx" : : "a" (data), "d" (port));
	#else
	port_out(port, (_size_t)data);
	#endif
}