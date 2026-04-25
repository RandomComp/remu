#ifndef _EMULATOR_OS_MEMORY_H
#define _EMULATOR_OS_MEMORY_H

#include "types.h"

void* get_ram();

uint8 in8(uint16 port);

uint16 in16(uint16 port);

uint32 in32(uint16 port);

void out8(uint16 port, uint8 data);

void out16(uint16 port, uint16 data);

void out32(uint16 port, uint32 data);

#endif
