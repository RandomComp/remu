#ifndef _EMULATOR_VGA_SCREEN_H
#define _EMULATOR_VGA_SCREEN_H

#include "types.h"

#include "screen/emulator_vga_fwd.h"

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL_image.h>
#endif

#define VGA_CHAR_WIDTH 8

#define VGA_CHAR_HEIGHT 16

typedef struct vga_text_screen_t {
	vga_text_device_t* vga_device;

	#ifdef EMULATOR_SDL_USING
	bool gui;

	byte* font;
	_size_t font_width;
	_size_t font_height;

	SDL_Texture* emulator_screen_texture;
	uint32* emulator_screen;
	_size_t emulator_screen_width;
	_size_t emulator_screen_height;
	#endif
} vga_text_screen_t;

void handle_mouse_move(size_t x, size_t y, int win_x, int win_y);

void handle_mouse_button(size_t x, size_t y, int win_x, int win_y, bool released);

#ifdef EMULATOR_SDL_USING
vga_text_screen_t* init_vga_text_screen(vga_text_device_t* vga_device, SDL_Texture* screen_texture, uint32* sdl_screen, _size_t screen_width, _size_t screen_height, bool gui);
#else
vga_text_screen_t* init_vga_text_screen(vga_text_device_t* vga_device);
#endif

void handle_copy_selected();

void draw_vga_text_screen(vga_text_screen_t* vga);

void vga_text_screen_apply_attribute(byte attribute);

void release_all_vga_text_screen(vga_text_screen_t* screen);

void reset_vga_text_screen(vga_text_screen_t* screen);

void free_vga_text_screen(vga_text_screen_t* screen);

#endif
