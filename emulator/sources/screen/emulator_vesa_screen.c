#include "types.h"

#include "screen/emulator_vesa_screen.h"

#include "emulator.h"

#include "emulator_logger.h"

#include <malloc.h>

#include <string.h>

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL_image.h>
#endif

static vesa_screen_t* cur = nullptr;

static void update_vesa_screen(void) {
	if (!cur) return;

	draw_vesa_screen(cur);
}

#ifdef EMULATOR_SDL_USING
vesa_screen_t* init_vesa_screen(SDL_Texture* texture, uint32* emulator_screen, size_t emulator_screen_width, size_t emulator_screen_height, vesa_device_t* vesa_device)
#else
vesa_screen_t* init_vesa_screen(vesa_device_t* vesa_device)
#endif
{
	#ifndef EMULATOR_SDL_USING
	emulator_log(true, LOG_SEVERITY_ERROR, "VESA is unsupported in non-gui mode");

	return nullptr;
	#endif

	vesa_screen_t* screen = malloc(sizeof(vesa_screen_t));

	memset(screen, 0, sizeof(vesa_screen_t));

	screen->texture = texture;
	screen->emulator_screen = emulator_screen;
	screen->emulator_screen_width = emulator_screen_width;
	screen->emulator_screen_height = emulator_screen_height;

	screen->vesa_device = vesa_device;

	cur = screen;

	emulator_setup_tick_timer(nullptr, update_vesa_screen, 33);

	return screen;
}

void draw_vesa_screen(vesa_screen_t* screen) {
	if (!screen) return;

	memcpy(screen->emulator_screen, screen->vesa_device->vidmem, screen->vesa_device->width * screen->vesa_device->height * screen->vesa_device->bpp / 8);

	SDL_UpdateTexture(screen->texture, null, screen->emulator_screen, screen->emulator_screen_width * sizeof(uint32));
}

void free_vesa_screen(vesa_screen_t* screen) {
	if (screen)
		free(screen);

	emulator_release_tick_timer(nullptr, update_vesa_screen);

	if (cur == screen)
		cur = nullptr;
}
