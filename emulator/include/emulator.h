#ifndef _EMULATOR_H
#define _EMULATOR_H

#include "cpu/emulator_cpu.h"

#include "pci/emulator_pci.h"

#include "time/emulator_pit.h"

#include "memory/emulator_ram.h"

#include "screen/emulator_vga_screen.h"
#include "screen/emulator_vga_gpu.h"

#include "screen/emulator_vesa_screen.h"
#include "screen/emulator_vesa_gpu.h"

#include "time/emulator_cmos.h"

#include "hid/emulator_kbdps2.h"

#include "ata/emulator_hdd_pio.h"

#include "power/emulator_power_control.h"

#include "emulator_multiboot.h"

#include "emulator_io.h"

#include "emulator_fwd.h"

#include "types.h"

#include <pthread.h>

typedef void (*tick_timer_handler_t)(void);

typedef void (*kmain_t)(uint32 magic, multiboot_info_t* multiboot);

typedef struct tick_timer_t {
	time_t last_time;

	time_t ms;

	tick_timer_handler_t handler;
} tick_timer_t;
#ifndef EMULATOR_SDL_USING
#define FRAMETIME_NS (1000 * 1000 * 20)

#define HALTED_FRAMETIME_NS (1000 * 1000 * 20)
#else
#define FRAMETIME_NS (1000 * 1000 * 10)

#define HALTED_FRAMETIME_NS (1000 * 1000 * 10)
#endif

#define TICK_TIMERS_SIZE_STEP (4)

#define EMULATOR_VERSION_STR "0.3.5"

#define EMULATOR_VERSION_FULL_STR EMULATOR_VERSION_STR " (" __DATE__ ", " __TIME__ ") for " PLATFORM_NAME " using " PLATFORM_COMPILER_NAME " %i.%i " PLATFORM_ARCH

#define EMULATOR_INFO_FULL_STR "OS Emulator, IBM-PC compatible (GPL V3.0) " EMULATOR_VERSION_FULL_STR " by RDev."

#define EMULATOR_INFO_STR "OS Emulator " EMULATOR_VERSION_STR " by RDev"

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL.h>
#endif

typedef struct __init_kernel_args_t {
	void* (*__emulator_get_ram)(void);

	_size_t (*__emulator_port_in)(uint16 port);

	void (*__emulator_port_out)(uint16 port, _size_t value);

	void (*__emulator_wait_halt)(void);

	void (*__emulator_sti)(void);

	void (*__emulator_cli)(void);

	uint64 (*__emulator_start_tsc)(void);

	void (*__emulator_idt_flush)(idt_ptr_t* ptr);

	void (*__emulator_kernel_report)(const byte* msg);
} __init_kernel_args_t;

typedef struct kernel_t {
	void (*__emulator_init_kernel)(__init_kernel_args_t* kernel_args);

	multiboot_section_t* (*__emulator_read_multiboot_secton)(void);

	void (*kmain)(uint32 magic, multiboot_info_t* multiboot);

	void* dl_handle;

	pthread_t kmain_thread; bool kmain_started; timer_t kmain_ints_exec_timer;
} kernel_t;

struct emulator_t {
	cpu_t* cpu;

	pci_t* pci;

	pit_t* pit;

	ram_t* ram;

	vga_text_screen_t* vga_screen;
	vga_text_device_t* vga_gpu;

	vesa_screen_t* vesa_screen;
	vesa_device_t* vesa_gpu;

	cmos_t* cmos;

	kbdps2_t* kbdps2;

	hdd_ata_pio_t* hdd;

	tick_timer_t* tick_timers;
	size_t tick_timers_cnt; 
	size_t tick_timers_size;

	uint64 ticks;

	uint64 emulator_start_time;

	bool is_hardware_reseting;

	bool gui;

	bool running;

	multiboot_info_t* multiboot_info;

	kernel_t* kernel;

	bool cleaned;

	#ifdef EMULATOR_SDL_USING
	SDL_Window* window;

	SDL_Renderer* renderer;

	SDL_Texture* screen_texture;

	uint32* screen;

	size_t screen_width; size_t screen_height;
	#endif
};

tick_timer_t emulator_setup_tick_timer(emulator_t* emulator, tick_timer_handler_t handler, time_t ms);

void emulator_release_tick_timer(emulator_t* emulator, tick_timer_handler_t handler);

void reset_emulator(emulator_t* emulator, int code);

void main_loop(emulator_t* _emulator, multiboot_section_t* multiboot_section);

kernel_t* emulator_load_kernel(const char* filename);

int emulator_unload_kernel(emulator_t* emulator);

pthread_t run_emulator(emulator_t* emulator);

void stop_emulator(emulator_t* emulator);

void emulator_forced_update_all_timers(emulator_t* emulator);

void emulator_update_all(emulator_t* emulator);

emulator_t* init_emulator(bool gui, _ssize_t width, _ssize_t height, _ssize_t frame_width, _ssize_t frame_height, _ssize_t bpp, bool vesa_mode, uint32 frametime_ns, uint32 halted_frametime_ns);

void free_emulator(emulator_t* emulator);

#endif
