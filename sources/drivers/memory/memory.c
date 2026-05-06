#include "drivers/memory/memory.h"

#include "types.h"

#include "multiboot.h"

#include "builtins/string.h"

#include "math/math.h"

#ifdef __EMULATOR__
#include "kernel.h"

extern __init_kernel_args_t kernel_args;
#endif

void* get_ram() {
	#ifndef __EMULATOR__
	return (void*)0;
	#else
	if (kernel_args.__emulator_get_ram)
		return kernel_args.__emulator_get_ram();
	#endif

	return nullptr;
}

size_t get_ram_size(multiboot_info_t* multiboot) {
	if (!multiboot) return 0;

	bool is_mem_available = (multiboot->flags & 0x01) != 0;

	size_t result = 0;

	if (is_mem_available) {
		result = (multiboot->mem_lower + (multiboot->mem_upper + 1024)) * 1024;
	}

	return result;
}

static byte ram_arr[0x2000] = { 0 };

// static byte ram_map[0x2000 / 8] = { 0 };

static byte* ram = nullptr;

#define RAM_MAP_ENABLE_BIT(bit_index) (ram_map[bit_index >> 3] |= 1ULL << (bit_index & 0x07))
#define RAM_MAP_DISABLE_BIT(bit_index) (ram_map[bit_index >> 3] &= ~(1ULL << (bit_index & 0x07)))

static ssize_t found_in_bitmap_best_fit(size_t size, byte* bitmap, size_t bitmap_size);
static ssize_t found_in_bitmap_fast_fit(byte* bitmap, size_t bitmap_size);

void init_ram(ssize_t size) {
	size = MIN(0x2000, size);

	size = size < 0 ? 0x2000 : size;

	ram = ram_arr;

	memset(ram, 0, size);
}

// typedef struct kmalloc_header_t {
// 	size_t mem_size;
// } kmalloc_header_t;

// Simple Bump Allocator

void* kmalloc(size_t size) {
	// size = align_up(size, 8);

	// size_t result_addr = found_in_bitmap_best_fit(size, ram_map, sizeof(ram_map));

	// for (size_t i = 0; i < align_up(size, 8) / 8; i++) {
	// 	ram_map[result_addr + i] = 0xFF;
	// }

	// for (size_t i = align_up(size, 8) / 8; i < size; i++) {
	// 	RAM_MAP_ENABLE_BIT(result_addr + i);
	// }

	// void* result = ram_arr + result_addr;

	// *((kmalloc_header_t*)result) = (kmalloc_header_t){ .mem_size = size };

	// return result + sizeof(kmalloc_header_t);

	void* result = ram;

	ram += size;

	return result;
}

void kfree(void* mem) {
	// kmalloc_header_t* mem_header = (kmalloc_header_t*)((byte*)mem - sizeof(kmalloc_header_t));

	// size_t result_addr = (uintmax_t)mem_header - (uintmax_t)ram_arr;

	// for (size_t i = 0; i < align_up(mem_header->mem_size, 8) / 8; i++) {
	// 	ram_map[result_addr + i] = 0;
	// }

	// for (size_t i = align_up(mem_header->mem_size, 8) / 8; i < mem_header->mem_size; i++) {
	// 	RAM_MAP_DISABLE_BIT(result_addr + i);
	// }
}

static ssize_t found_in_bitmap_fast_fit(byte* bitmap, size_t bitmap_size) {
	size_t start = 0;

	bool ok = false;

	for (size_t i = 0; i < bitmap_size * 8; i++) {
		byte map_byte = bitmap[i / 8];

		if (map_byte >= 0xFF) continue;

		byte sector_map_bit = map_byte & (1 << (i % 8));

		if (!sector_map_bit) {
			ok = true; break;
		}
	}

	return ok ? start : -1;
}

static ssize_t found_in_bitmap_best_fit(size_t size, byte* bitmap, size_t bitmap_size) {
	size_t start = 0; size_t available_size = 0;

	bool ok = false;

	for (size_t i = 0; i < bitmap_size * 8; i++) {
		byte map_byte = bitmap[i / 8];

		if (map_byte >= 0xFF) continue;

		byte sector_map_bit = map_byte & (1 << (i % 8));

		if (!sector_map_bit) {
			if (start == 0)
				start = i;
			
			available_size++;
		}

		else {
			start = 0; available_size = 0;
		}

		if (available_size >= size) {
			ok = true; break;
		}
	}

	return ok ? start : -1;
}
