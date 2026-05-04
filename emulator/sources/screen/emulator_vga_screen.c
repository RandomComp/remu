#include "types.h"

#include "emulator_logger.h"

#include "main.h"

#include "emulator.h"

#include "utils.h"

#include "screen/emulator_vga_screen.h"

#include "screen/emulator_vga_gpu.h"

#include "colors.h"

#include "ansi.h"

#include <stdint.h>

#include <malloc.h>

#include <string.h>

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL_image.h>
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(x, min_value, max_value) (MAX(MIN(x, max_value), min_value))

extern const c_str cp437[];

static vga_text_screen_t* cur = nullptr;

static void update_cur_vga_screen() {
	if (!cur) return;

	draw_vga_text_screen(cur);
}

static bool blinking_text_visible = false;

static void text_blink_update() {
	blinking_text_visible = !blinking_text_visible;
}

#ifdef EMULATOR_SDL_USING
static byte* load_png_font(const char* file_name, size_t* w, size_t* h) {
	if (!file_name) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot load png font: file_name is NULL");

		return nullptr;
	}

	emulator_log(true, LOG_SEVERITY_INFO, "Loading cp437 font \"%s\"...", file_name);

	int width = -1, height = -1;

	SDL_Surface* temp = IMG_Load(file_name);

	if (!temp) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot load png font: %s", SDL_GetError());

		return nullptr;
	}

	SDL_Surface* temp2 = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);

	SDL_FreeSurface(temp);

	if (!temp2) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot load png font: %s", SDL_GetError());

		return nullptr;
	}

	width = temp2->w;

	height = temp2->h;

	byte* font = malloc(width * height);

	if (!font) {
		SDL_FreeSurface(temp2);

		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot load png font: cannot alloc array of pixels for font (malloc returned NULL)");

		return nullptr;
	}

	memset(font, 0, width * height);

	uint32* pixels = temp2->pixels;

	for (size_t i = 0; i < width * height; i++) {
		size_t index = i / 8; size_t bit_index = i % 8;

		uint32 pixel = pixels[i];

		uint32 r = pixel & 0xFF;

		uint32 g = (pixel >> 8) & 0xFF;

		uint32 b = (pixel >> 16) & 0xFF;

		uint32 gray = (r + g + b) / 3;

		if (gray >= 128) {
			font[index] |= 1 << bit_index;
		}
	}

	SDL_FreeSurface(temp2);

	if (w) *w = width;

	if (h) *h = height;

	return font;
}
#endif

#ifdef EMULATOR_SDL_USING
vga_text_screen_t* init_vga_text_screen(vga_text_device_t* vga_device, SDL_Texture* screen_texture, uint32* sdl_screen, _size_t screen_width, _size_t screen_height, bool gui)
#else
vga_text_screen_t* init_vga_text_screen(vga_text_device_t* vga_device)
#endif
{
	vga_text_screen_t* screen = malloc(sizeof(vga_text_screen_t));

	memset(screen, 0, sizeof(vga_text_screen_t));

	screen->vga_device = vga_device;
	
	#ifdef EMULATOR_SDL_USING

	screen->gui = gui;

	screen->emulator_screen_texture = screen_texture;

	screen->emulator_screen = sdl_screen;

	screen->emulator_screen_width = screen_width;

	screen->emulator_screen_height = screen_height;

	screen->font = load_png_font("cp437-8x16.png", &screen->font_width, &screen->font_height);

	if (!screen->font) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot load emulator cp437 font");

		free(screen);

		imd_exit_emulator(1);

		return nullptr;
	}

	#endif

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up VGA screen update timer (33 hz) and text blinking timer (2 hz)...");

	cur = screen;

	emulator_setup_tick_timer(nullptr, update_cur_vga_screen, 33);
	emulator_setup_tick_timer(nullptr, text_blink_update, 500);

	#ifdef EMULATOR_SDL_USING
	if (!screen->gui)
		printf(ansi_clear_screen);
	#else
	printf(ansi_clear_screen);
	#endif
}

#ifdef EMULATOR_SDL_USING
static int draw_vga_ch(vga_text_screen_t* screen, byte ch, bool bg_transparent, uint32 bg_color, uint32 fg_color, size_t column, size_t row) {
	if (!screen || !screen->vga_device->vidmem) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw character using uninitialized vga text screen!");

		return 1;
	}

	if (!screen->gui) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw character using non-gui vga text screen!");

		return 1;
	}

	size_t screen_pos = column + (row * screen->vga_device->width);

	if (screen_pos >= (screen->vga_device->width * screen->vga_device->height)) return 1;

	column = screen_pos % screen->vga_device->width;

	row = screen_pos / screen->vga_device->width;

	size_t font_grid_width = screen->font_width / VGA_CHAR_WIDTH;

	size_t screen_x = column * VGA_CHAR_WIDTH;
	size_t screen_y = row * VGA_CHAR_HEIGHT;

	size_t font_column = ch % font_grid_width;
	size_t font_row = ch / font_grid_width;
	
	for (size_t i = 0; i < VGA_CHAR_HEIGHT; i++) {
		size_t index = (font_column * VGA_CHAR_WIDTH) + (((font_row * VGA_CHAR_HEIGHT) + i) * screen->font_width);

		index /= 8;

		byte column_pixels = screen->font[index];

		for (size_t j = 0; j < VGA_CHAR_WIDTH; j++) {
			bool bit_enabled = column_pixels & (1 << j);

			size_t screen_index = screen_x + j + ((screen_y + i) * screen->emulator_screen_width);

			if (!bg_transparent) {
				screen->emulator_screen[screen_index] = bit_enabled ? fg_color : bg_color;
			}

			else if (bit_enabled) {
				screen->emulator_screen[screen_index] = fg_color;
			}
		}
	}

	return 0;
}

static size_t inverse_start = 0, inverse_end = 0;

static bool selecting = false;

void handle_mouse_move(size_t x, size_t y, int win_x, int win_y) {
	if (!cur) return;

	if (!selecting) return;

	size_t cursor_x = (size_t)(x * 80) / win_x;

	size_t cursor_y = (size_t)(y * 25) / win_y;

	inverse_end = cursor_x + (cursor_y * cur->vga_device->width);
}

void handle_mouse_button(size_t x, size_t y, int win_x, int win_y, bool released) {
	if (!cur) return;

	size_t cursor_x = (size_t)(x * 80) / win_x;

	size_t cursor_y = (size_t)(y * 25) / win_y;

	if (!released) {
		inverse_start = cursor_x + (cursor_y * cur->vga_device->width);

		inverse_end = inverse_start;
	}

	selecting = !released;
}

void handle_copy_selected() {
	if (!cur) return;

	size_t start = MIN(inverse_start, inverse_end);
	size_t end = MAX(inverse_start, inverse_end);
	
	size_t columns_start = start % cur->vga_device->width;
	size_t rows_start = MIN(cur->vga_device->height - 1, start / cur->vga_device->width);

	size_t columns_end = end % cur->vga_device->width;
	size_t rows_end = MIN(cur->vga_device->height - 1, end / cur->vga_device->width);

	char* buf = malloc(cur->vga_device->width * cur->vga_device->height * 3);

	memset(buf, 0, cur->vga_device->width * cur->vga_device->height * 3);

	size_t text_to_copy_index = 0;

	for (size_t j = rows_start; j <= rows_end; j++) {
		size_t index = cur->vga_device->width;

		bool ok = false;

		for (; index >= columns_start; index--) {
			byte b = cur->vga_device->vidmem[(j * cur->vga_device->width) + index] & 0xFF;

			if ((b != ' ') && (b != '\0')) {
				ok = true;
				
				break;
			}
		}

		if (!ok) break;

		bool is_first_j = j == rows_start;

		bool is_last_j = j == rows_end;

		for (size_t i = (is_first_j ? columns_start : 0); i < (is_last_j ? columns_end : index); i++) {
			c_str cp437_c = cp437[cur->vga_device->vidmem[(j * cur->vga_device->width) + i] & 0xFF];

			size_t cp437_c_len = strlen(cp437_c);

			memcpy(buf + text_to_copy_index, cp437_c, cp437_c_len);

			text_to_copy_index += cp437_c_len;
		}

		buf[text_to_copy_index++] = '\n';
	}

	SDL_SetClipboardText(buf);

	free(buf);
}

const uint32 vga_color_to_rgb[] = {
	[COLOR_BLACK] = 			0x000000,
	[COLOR_BLUE] = 				0x000080,
	[COLOR_GREEN] = 			0x008000,
	[COLOR_AQUA] = 				0x008080,
	[COLOR_RED] = 				0x800000,
	[COLOR_MAGENTA] = 			0x800080,
	[COLOR_BROWN] = 			0xAA5000,
	[COLOR_WHITE] = 			0x808080,
	[COLOR_BRIGHT_BLACK] = 		0x404040,
	[COLOR_BRIGHT_BLUE] = 		0x0000FF,
	[COLOR_BRIGHT_LIME] = 		0x00FF00,
	[COLOR_BRIGHT_AQUA] = 		0x00FFFF,
	[COLOR_BRIGHT_RED] = 		0xFF0000,
	[COLOR_BRIGHT_MAGENTA] = 	0xFF00FF,
	[COLOR_BRIGHT_YELLOW] = 	0xFFFF00,
	[COLOR_BRIGHT_WHITE] = 		0xFFFFFF
};

const uint32 inversed_vga_color_to_rgb[] = {
	[COLOR_BLACK] = 			0xFFFFFF,
	[COLOR_BLUE] = 				0xFFFF7F,
	[COLOR_GREEN] = 			0xFF7FFF,
	[COLOR_AQUA] = 				0xFF7F7F,
	[COLOR_RED] = 				0x7FFFFF,
	[COLOR_MAGENTA] = 			0x7FFF7F,
	[COLOR_BROWN] = 			0x55AFFF,
	[COLOR_WHITE] = 			0x808080,
	[COLOR_BRIGHT_BLACK] = 		0xC0C0C0,
	[COLOR_BRIGHT_BLUE] = 		0xFFFF00,
	[COLOR_BRIGHT_LIME] = 		0xFF00FF,
	[COLOR_BRIGHT_AQUA] = 		0xFF0000,
	[COLOR_BRIGHT_RED] = 		0x00FFFF,
	[COLOR_BRIGHT_MAGENTA] = 	0x00FF00,
	[COLOR_BRIGHT_YELLOW] = 	0x0000FF,
	[COLOR_BRIGHT_WHITE] = 		0x000000
};

static int draw_vga_text_screen_gui(vga_text_screen_t* vga) {
	if (!vga || !vga->vga_device || !vga->vga_device->vidmem) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw uninitialized vga text screen device!");

		return 1;
	}

	if (!vga->gui) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw character using non-gui vga text screen device!");

		return 1;
	}

	for (size_t i = 0; i < vga->vga_device->width * vga->vga_device->height; i++) {
		if ((vga->vga_device->mode_reg & 0x20) == 0) {
			draw_vga_ch(vga, ' ', false, 0, 0, i, 0);
		}

		uint16 ch_and_style = vga->vga_device->vidmem[i];

		byte ch = ch_and_style & 0xFF;

		byte style = (ch_and_style >> 8) & 0xFF;

		byte fg_style = style & 0xF;

		byte bg_style = (style >> 4) & 0xF;

		bool bg_bright = bg_style & 0x8;
			
		if ((vga->vga_device->mode_reg & 0x08) != 0) {
			bg_style = bg_style % 8;

			if (bg_bright && !blinking_text_visible) {
				draw_vga_ch(vga, ' ', false, 0, 0, i, 0);
				
				continue;
			}
		}

		uint32 fg_rgb_color = vga_color_to_rgb[fg_style];

		uint32 bg_rgb_color = vga_color_to_rgb[bg_style];

		if ((i >= inverse_start && i < inverse_end) ||
			(i >= inverse_end && i < inverse_start)) {
			fg_rgb_color = vga_color_to_rgb[15 - fg_style];

			bg_rgb_color = vga_color_to_rgb[15 - bg_style];
		}

		bool transparent = false;

		if (i == vga->vga_device->cursor_pos && blinking_text_visible) {
			draw_vga_ch(vga, '_', false, 0, 0xFFFFFF, i, 0);

			transparent = true;
		}
			
		draw_vga_ch(vga, ch, transparent, bg_rgb_color, fg_rgb_color, i, 0);
	}

	return 0;
}
#endif

static void draw_vga_text_screen_cli(vga_text_screen_t* screen) {
	if (!screen || !screen->vga_device->vidmem) return;

	_ssize_t columns, rows;

	get_terminal_size(&columns, &rows);

	_size_t width = screen->vga_device->width < columns ? screen->vga_device->width : columns;

	_size_t height = screen->vga_device->height < rows ? screen->vga_device->height : rows;

	set_cursor_pos(0, 0);

	printf(default_screen_color bold white_fg);

	for (size_t i = 0; i < height; i++) {
		bool is_last_i = i == (height - 1);

		for (size_t j = 0; j < width; j++) {
			if ((screen->vga_device->mode_reg & 0x20) == 0) {
				putchar(' ');

				continue;
			}

			_size_t ch_pos = j + (i * width);

			if (ch_pos == screen->vga_device->cursor_pos &&
				blinking_text_visible) {
				printf(underline);
			}

			uint16 ch_and_style = screen->vga_device->vidmem[ch_pos];

			byte style = (ch_and_style >> 8) & 0xFF;

			byte style_bg = (style >> 4) & 0xF;
			
			bool bg_bright = style_bg & 8;
			
			bool text_visible = ((screen->vga_device->mode_reg & 0x08) == 0) || (!bg_bright || blinking_text_visible);

			if (text_visible) {
				byte c = ch_and_style & 0xFF;

				vga_text_screen_apply_attribute(style);

				printf("%s", cp437[c]);

				printf(default_screen_color);
			}
			
			else putchar(' ');
		}

		if (!is_last_i) printf("\n\r");
	}

	printf(default_screen_color);

	// fflush(stdout);
}

void draw_vga_text_screen(vga_text_screen_t* vga) {
	#ifdef EMULATOR_SDL_USING
	if (!vga->gui) {
		draw_vga_text_screen_cli(vga);
	}

	else {
		draw_vga_text_screen_gui(vga);

		SDL_UpdateTexture(vga->emulator_screen_texture, null, vga->emulator_screen, vga->emulator_screen_width * sizeof(uint32));
	}
	#else
	draw_vga_text_screen_cli(vga);
	#endif
}

static void vga_text_screen_apply_attribute_no_gui(byte attribute) {
	byte style_fg = attribute & 0xF;
	
	bool fg_bright = style_fg & 8;

	byte style_bg = (attribute >> 4) & 0xF;
	
	bool bg_bright = style_bg & 8;

	byte ansi_code_fg = color_to_ansi(style_fg % 8);

	byte ansi_code_bg = color_to_ansi(style_bg % 8);

	if (fg_bright) {
		ansi_code_fg += 60;
	}

	if (bg_bright) {
		if ((cur->vga_device->mode_reg & 0x8) == 0) {
			ansi_code_bg += 60;
		}
	}

	if (style_fg == COLOR_BLACK || ansi_code_fg == 0) {
		printf(bright_black_fg);
	}

	else if (style_fg == COLOR_BRIGHT_BLACK) {
		printf(bold gray_fg);
	}

	else if (fg_bright) {
		printf("\x1B[%i;1m", (int)ansi_code_fg);
	}
	
	else {
		printf("\x1B[%im", (int)ansi_code_fg);
	}

	if (style_bg == COLOR_BLACK || ansi_code_bg == 0) {
		printf(bright_black_bg);
	}

	else if (style_bg == COLOR_BRIGHT_BLACK) {
		printf(bold gray_bg);
	}

	// else if (bg_bright && (cur->mode_reg & 0x8) == 0) {
	// 	printf("\x1B[%i;1m", (int)ansi_code_bg + 10);
	// }

	else {
		printf("\x1B[%im", (int)ansi_code_bg + 10);
	}
}

void vga_text_screen_apply_attribute(byte attribute) {
	if (!cur) return;

	#ifdef EMULATOR_SDL_USING
	if (!cur->gui)
		vga_text_screen_apply_attribute_no_gui(attribute);
	#else
	vga_text_screen_apply_attribute_no_gui(attribute);
	#endif
}

void release_all_vga_text_screen(vga_text_screen_t* screen) {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Releasing timer updating vga screen (33 hz) and text blinking timer (2 hz)...");

	emulator_release_tick_timer(nullptr, update_cur_vga_screen);

	emulator_release_tick_timer(nullptr, text_blink_update);
}

void reset_vga_text_screen(vga_text_screen_t* screen) {
	#ifdef EMULATOR_SDL_USING
	if (!screen->gui) {
		printf(default_style);

		printf(ansi_clear_screen);
	}
	#else
	printf(default_style);

	printf(ansi_clear_screen);
	#endif
}

void free_vga_text_screen(vga_text_screen_t* screen) {
	#ifdef EMULATOR_SDL_USING
	if (!screen->gui) {
		printf(default_style);

		printf(ansi_clear_screen);
	}

	else if (screen->font) {
		emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA screen font deinitialization...");

		free(screen->font);

		screen->font = nullptr;

		emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA screen font deinitialized!");
	}
	#else
	printf(default_style);

	printf(ansi_clear_screen);
	#endif
}
