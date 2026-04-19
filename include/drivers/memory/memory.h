#ifndef _EMULATOR_OS_MEMORY_H
#define _EMULATOR_OS_MEMORY_H

#include "types.h"

typedef enum PredefinedMemoryType {
    PMT_RAM_BASIC,      // Этим типом помечается обычная память. ( не графическая )
    PMT_TEXT_MEM_80x25  // Тип для получения текстовой памяти режима 80x25.
} PredefinedMemoryType;

typedef enum RealMemoryAddresses {
    RMA_RAM_BASIC =         0x200000,
    RMA_TEXT_MEM_80x25 =    0x0B8000
} RealMemoryAddresses;

void* getMemoryOffset(PredefinedMemoryType EMAType);

uint8 in8(uint16 port);

uint16 in16(uint16 port);

uint32 in32(uint16 port);

void out8(uint16 port, uint8 data);

void out16(uint16 port, uint16 data);

void out32(uint16 port, uint32 data);

#endif