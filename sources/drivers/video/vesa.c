#include "drivers/video/vesa.h"

#include "types.h"

#include "terminal.h"

#include "colors.h"

#include "builtins/string.h"

static size_t cur_x = 0, cur_y = 0;

static byte* 	fb = nullptr;
static size_t 	fb_w = 0, fb_h = 0, fb_bpp = 0, fb_pitch = 0;

static byte font[256 * 16] = {
	#include "cp437.fnt"
};

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

static void draw_ch(uint32 bg_color, uint32 fg_color, size_t x, size_t y, byte ch) {
	if (!fb) return;

	for (size_t i = 0; i < 16; i++) {
		for (size_t j = 0; j < 8; j++) {
			size_t pos = (x * 8) + j + ((i + (y * 16)) * fb_w);

			if (pos >= (fb_w * fb_h)) {
				return;
			}

			bool enabled = font[(ch * 16) + i] & (1ULL << (7 - j));

			size_t bytes = fb_bpp / 8;

			if (enabled) {
				if (fb_bpp == 32) {
					fb[pos * bytes + 0] = (fg_color >> 0) 	& 0xFF;
					fb[pos * bytes + 1] = (fg_color >> 8) 	& 0xFF;
					fb[pos * bytes + 2] = (fg_color >> 16) 	& 0xFF;
					fb[pos * bytes + 3] = (fg_color >> 24) 	& 0xFF;
				}

				else if (fb_bpp == 16) {
					fb[pos * bytes + 0] = (fg_color >> 0) 	& 0xFF;
					fb[pos * bytes + 1] = (fg_color >> 8) 	& 0xFF;
				}

				else {
					fb[pos * bytes + 0] = (fg_color >> 0) 	& 0xFF;
				}
			}

			else {
				if (fb_bpp == 32) {
					fb[pos * bytes + 0] = (bg_color >> 0) 	& 0xFF;
					fb[pos * bytes + 1] = (bg_color >> 8) 	& 0xFF;
					fb[pos * bytes + 2] = (bg_color >> 16) 	& 0xFF;
					fb[pos * bytes + 3] = (bg_color >> 24) 	& 0xFF;
				}

				else if (fb_bpp == 16) {
					fb[pos * bytes + 0] = (bg_color >> 0) 	& 0xFF;
					fb[pos * bytes + 1] = (bg_color >> 8) 	& 0xFF;
				}

				else {
					fb[pos * bytes + 0] = (bg_color >> 0) 	& 0xFF;
				}
			}
		}
	}
}

static void vesa_putch(byte style, byte c) {
	size_t width = fb_w/ 8;
	size_t height = fb_h / 16;
	
	ssize_t cur_pos = (cur_y * 8) + cur_x;

	if (cur_pos < 0) {
		return;
	}

	uint32 fg_color = vga_color_to_rgb[style & 0xF];
	uint32 bg_color = vga_color_to_rgb[(style >> 4) & 0xF];
	
	switch (c) {
		case '\b':
			cur_x -= 1; break;

		case '\t':
			cur_x += 4; break;
		
		case '\n':
			cur_y += 1; break;
		
		case '\r':
			cur_x = 0; break;
		
		default:
			if (cur_pos >= 0 && cur_pos < (width * height)) {
				draw_ch(bg_color, fg_color, cur_x, cur_y, c);
			}

			cur_x += 1;

			return;
	}
	
	if (cur_pos >= 0 && cur_pos < (width * height)) {
		draw_ch(0x000000, 0x000000, cur_x, cur_y, ' ');
	}
}

static void vesa_set_cur_pos(ssize_t column, ssize_t row, bool view) {
	if (view) return;

	cur_x = column;
	cur_y = row;
}

static void vesa_get_cur_pos(ssize_t* column, ssize_t* row) {
	if (column) *column = cur_x;
	if (row) 	*row 	= cur_y;
}

terminal_out_t init_vesa_stdout(void* _fb, size_t _fb_w, size_t _fb_h, size_t _fb_bpp, size_t _fb_pitch) {
	fb = (byte*)_fb; fb_w = _fb_w; fb_h = _fb_h; fb_bpp = _fb_bpp; fb_pitch = _fb_pitch;

	terminal_out_t result = { 0 };

	result.putch 		= vesa_putch;
	result.columns 		= _fb_w / 8;
	result.rows 		= _fb_h / 16;
	result.set_cur_pos 	= vesa_set_cur_pos;
	result.get_cur_pos 	= vesa_get_cur_pos;

	return result;
}
