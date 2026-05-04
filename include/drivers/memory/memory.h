#ifndef _EMULATOR_OS_MEMORY_H
#define _EMULATOR_OS_MEMORY_H

#include "types.h"

#include "multiboot.h"

void* get_ram();

size_t get_ram_size(multiboot_info_t* multiboot);

void init_ram(ssize_t size);

void* kmalloc(size_t size);

void kfree(void* mem);

#endif
