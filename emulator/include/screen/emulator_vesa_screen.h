#ifndef _EMULATOR_VESA_SCREEN_H
#define _EMULATOR_VESA_SCREEN_H

#include "types.h"

#include "screen/emulator_vesa_gpu.h"

#include "emulator_fwd.h"

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL_image.h>
#endif

typedef struct vesa_screen_t {
	#ifdef EMULATOR_SDL_USING
	SDL_Texture* texture;
	#endif

	uint32* emulator_screen;
	size_t emulator_screen_width;
	size_t emulator_screen_height;

	vesa_device_t* vesa_device;
} vesa_screen_t;

#ifdef EMULATOR_SDL_USING
vesa_screen_t* init_vesa_screen(SDL_Texture* texture, uint32* emulator_screen, size_t emulator_screen_width, size_t emulator_screen_height, vesa_device_t* vesa_device);
#else
vesa_screen_t* init_vesa_screen(vesa_device_t* vesa_device);
#endif

void draw_vesa_screen(vesa_screen_t* screen);

void free_vesa_screen(vesa_screen_t* screen);

#endif
