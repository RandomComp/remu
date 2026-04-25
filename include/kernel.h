#ifndef _EMULATOR_OS_KERNEL_H
#define _EMULATOR_OS_KERNEL_H

#include "types.h"

#include "multiboot.h"

void kmain(uint32 magic, multiboot_info_t* ptr);

#endif
