#include "memory/emulator_ram.h"

#include "types.h"

#include "main.h"

#include "emulator_logger.h"

#include <stdlib.h>

#include <string.h>

#ifdef IS_UNIX
#include <sys/mman.h>
#endif

ram_t* init_ram(size_t mem_size) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "RAM initialization (%zx bytes)...", mem_size);

	ram_t* ram = malloc(sizeof(ram_t));

	memset(ram, 0, sizeof(ram_t));

	ram->mem_size = mem_size;

	#ifdef IS_UNIX
	ram->mem_ptr = mmap(null, ram->mem_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	#else
	ram->mem_ptr = malloc(ram->mem_size);
	#endif

	memset(ram->mem_ptr, 0, ram->mem_size);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "RAM initialized!");

	return ram;
}

void reset_ram(ram_t* ram) {
	if (!ram || !ram->mem_ptr || ram->mem_size <= 0) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "RAM reseting...");

	memset(ram->mem_ptr, 0, ram->mem_size);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "RAM reseted!");
}

void free_ram(ram_t* ram) {
	if (!ram) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "RAM deinitialization...");

	#ifdef IS_UNIX
	if (ram->mem_ptr && ram->mem_size > 0) {
		munmap(ram->mem_ptr, ram->mem_size);
	}
	#else
	if (ram->mem_ptr) free(ram->mem_ptr);
	#endif

	ram->mem_ptr = nullptr;

	free(ram);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "RAM deinitialized");
}
