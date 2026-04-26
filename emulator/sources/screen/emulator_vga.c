#include "screen/emulator_vga.h"

#include "types.h"

#include "colors.h"

#include "ansi.h"

#include "utils.h"

#include "main.h"

#include "emulator.h"

#include "emulator_io.h"

#include "emulator_logger.h"

#include "math.h"

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#ifdef EMULATOR_SDL_USING
#include <SDL2/SDL_image.h>
#endif

const c_str cp437[] = {
	" ", "☺", "☻", "♥", "♦", "♣", 
	"♠", "•", "◘", "○", "◙", "♂", 
	"♀", "♪", "♫", "☼", "►", "◄", 
	"↕", "‼", "¶", "§", "▬", "↨", 
	"↑", "↓", "→", "←", "∟", "↔", 
	"▲", "▼", " ", "!", "\"", "#", 
	"$", "%", "&", "\'", "(", ")", 
	"*", "+", ",", "-", ".", "/", 
	"0", "1", "2", "3", "4", "5", 
	"6", "7", "8", "9", ":", ";", 
	"<", "=", ">", "?", "@", "A", 
	"B", "C", "D", "E", "F", "G", 
	"H", 
	"I", "J", "K", "L", "M", "N", 
	"O", "P", "Q", "R", "S", "T", 
	"U", "V", "W", "X", "Y", "Z", 
	"[", "\\", "]", "^", "_", "`", 
	"a", 
	"b", "c", "d", "e", "f", "g", 
	"h", "i", "j", "k", "l", "m", 
	"n", "o", "p", "q", "r", "s", 
	"t", "u", "v", "w", "x", "y", 
	"z", "{", "¦", "}", "~",
	"⌂", "Ç", "ü", "é", "â", "ä", 
	"à", "å", "ç", "ê", "ë", "è", 
	"ï", "î", "ì", "Ä", "Å", "É", 
	"æ", "Æ", "ô", "ö", "ò", "û", 
	"ù", "ÿ", "Ö", "Ü", "¢", "£", 
	"¥", "₧", "ƒ", "á", "í", "ó", 
	"ú", "ñ", "Ñ", "ª", "º", "¿", 
	"⌐", "¬", "½", "¼", "¡", "«", 
	"»", "░", "▒", "▓", "│", "┤", 
	"╡", "╢", "╖", "╕", "╣", "║", 
	"╗", "╝", "╜", "╛", "┐", "└", 
	"┴", "┬", "├", "─", "┼", "╞", 
	"╟", "╚", "╔", "╩", "╦", "╠", 
	"═", "╬", "╧", "", "╨", "╤", 
	"╥", "╙", "╘", "╒", "╓", "╫", 
	"╪", "┘", "┌", "█", "▄", "▌", 
	"▐", "▀", "α", "ß", "Γ", "π", 
	"Σ", "σ", "µ", "τ", "Φ", "Θ", 
	"Ω", "δ", "∞", "φ", "ε", "∩", 
	"≡", "±", "≥", "≤", "⌠", "⌡", 
	"÷", "≈", "°", "∙", "·", "√", 
	"ⁿ", "²", "■", " "
};

static vga_text_screen_t* cur = nullptr;

static bool attrib_flip_trigger_flag = false;

static byte attrib_reg = 0, display_vidmem = 0;

static _size_t attrib_flip_trigger() {
	if (!cur) return 0;

	attrib_flip_trigger_flag = true;

	emulator_log(false, LOG_SEVERITY_INFO, "Write addr/data flag triggered (0x3DA) for 0x3C0 in VGA text screen");

	return 0;
}

static void attrib_reg_write(_size_t data) {
	if (!cur) return;

	if (attrib_flip_trigger_flag) {
		attrib_reg = data & 0x1F;

		display_vidmem |= data & 0x20;

		attrib_flip_trigger_flag = false;

		emulator_log(false, LOG_SEVERITY_INFO, "Writed %llx as register to 0x3C0 (VGA text screen)\r", attrib_reg);
	}

	else if (attrib_reg & 0x10) {
		cur->mode_reg = data & (~0x20);

		display_vidmem |= data & 0x20;

		if (display_vidmem != 0)
			cur->mode_reg |= 0x20;
		else
			cur->mode_reg &= ~0x20;

		display_vidmem = 0;
		
		attrib_reg = 0;

		emulator_log(false, LOG_SEVERITY_INFO, "Writed %llx as data to 0x3C0 (VGA text screen)\r", data);
	}
}

static _size_t attrib_reg_read() {
	if (!cur) return 0;

	if (!attrib_flip_trigger_flag && attrib_reg == 0x10) {
		attrib_reg = 0;

		emulator_log(false, LOG_SEVERITY_INFO, "VGA text screen mode register readed (0x3C1)");

		return cur->mode_reg;
	}

	attrib_flip_trigger_flag = false;

	display_vidmem = 0;

	return 0;
}

static _size_t crt_reg = 0;

static void crt_select_reg(_size_t data) {
	if (!cur) return;

	crt_reg = data & 0xFF;
}

static void crt_write_reg(_size_t data) {
	if (!cur) return;

	if (crt_reg == 0xE) {
		cur->cursor_pos = (cur->cursor_pos & 0xFFFF00FF) | ((data & 0xFF) << 8);
	}
	
	else if (crt_reg == 0xF) {
		cur->cursor_pos = (cur->cursor_pos & 0xFFFFFF00) | (data & 0xFF);
	}
}

static _size_t crt_read_reg() {
	if (!cur) return 0;

	if (crt_reg == 0xA) {
		return cur->cursor_pos & 0x0000FF00;
	}
	
	else if (crt_reg == 0xE) {
		return cur->cursor_pos & 0x0000FF00;
	}
	
	else if (crt_reg == 0xF) {
		return cur->cursor_pos & 0x000000FF;
	}
	
	return 0;
}

// static byte sequence_sel_reg = 0;

// static void sequence_reg_select(_size_t reg) {
// 	reg = reg & 0xFF;

// 	if (reg == )
// }

// static void sequence_reg_write_data(_size_t data) {

// }

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
vga_text_screen_t* init_vga_text_screen(SDL_Texture* screen_texture, uint32* screen, _size_t screen_width, _size_t screen_height, bool gui, ram_t* ram, _ssize_t columns, _ssize_t rows)
#else
vga_text_screen_t* init_vga_text_screen(ram_t* ram, _ssize_t columns, _ssize_t rows)
#endif
{
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text screen initialization...");

	size_t vidmem_size = columns * rows * sizeof(uint16);

	uint64 vidmem_ram_addr = 0xB8000;

	if (!ram || !ram->mem_ptr) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize VGA because RAM is not initialized!"); 
		
		return nullptr;
	}

	if (ram->mem_size < (vidmem_ram_addr + vidmem_size)) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize VGA because RAM is lower than vidmem address end %llx!", vidmem_ram_addr + vidmem_size); 
		
		return nullptr;
	}

	vga_text_screen_t* vga = malloc(sizeof(vga_text_screen_t));

	memset(vga, 0, sizeof(vga_text_screen_t));

	vga->vidmem_ram_addr = vidmem_ram_addr;

	vga->vidmem = (uint16*)(ram->mem_ptr + vga->vidmem_ram_addr);

	vga->width = columns; vga->height = rows; vga->bpp = 2 * 8;

	vga->cursor_pos = 0;

	vga->mode_reg = 0b00001000 | 0x20;

	vga->crt_reg_a = 0b00000000;

	#ifdef EMULATOR_SDL_USING

	vga->gui = gui;

	vga->emulator_screen_texture = screen_texture;

	vga->emulator_screen = screen;

	vga->emulator_screen_width = screen_width;

	vga->emulator_screen_height = screen_height;

	vga->font = load_png_font("cp437-8x16.png", &vga->font_width, &vga->font_height);

	if (!vga->font) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot load emulator cp437 font");

		free(vga);

		imd_exit_emulator(1);

		return nullptr;
	}

	#endif
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "VGA text screen mode register (no video mode): 0b%08b\r", vga->mode_reg);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Clearing VGA text screen...");

	clear_vga_text_screen(vga);

	cur = vga;
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up VGA screen update timer (33 hz) and text blinking timer (2 hz)...");

	emulator_setup_tick_timer(nullptr, update_cur_vga_screen, 33);
	emulator_setup_tick_timer(nullptr, text_blink_update, 500);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up ports (0x3DA, 0x3C0, 0x3C1) for VGA...");

	emulator_setup_port_in(0x3DA, attrib_flip_trigger);
	emulator_setup_port_out(0x3C0, attrib_reg_write);
	emulator_setup_port_in(0x3C1, attrib_reg_read);

	emulator_setup_port_out(0x3D4, crt_select_reg);
	emulator_setup_port_out(0x3D5, crt_write_reg);
	emulator_setup_port_in(0x3D5, crt_read_reg);

	// emulator_setup_port_out(0x3C4, sequence_reg_select);
	// emulator_setup_port_out(0x3C5, sequence_reg_write_data);

	// emulator_setup_port_out(0x3CE, sequence_reg_select);
	// emulator_setup_port_out(0x3CF, sequence_reg_write_data);
	
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text screen initialized");

	#ifdef EMULATOR_SDL_USING
	if (!vga->gui)
		printf(ansi_clear_screen);
	#else
	printf(ansi_clear_screen);
	#endif

	return vga;
}

void clear_vga_text_screen(vga_text_screen_t* screen) {
	if (!screen || !screen->vidmem) return;

	for (size_t i = 0; i < screen->width * screen->height; i++) {
		screen->vidmem[i] = ' ' | (0x0F << 8);
	}
}

int draw_vga_text(vga_text_screen_t* vga, const c_str text, byte style, _size_t column, _size_t row) {
	if (!vga || !vga->vidmem) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw text using uninitialized vga text screen device!");

		return 1;
	}

	if (!text) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw text with NULL pointer!");

		return 1;
	}

	size_t remaining = (vga->width * vga->height) - (column + (row * vga->width));

	for (size_t i = 0; text[i] && i < remaining; i++) {
		vga->vidmem[i + column + (row * vga->width)] = text[i] | (style << 8);
	}

	return 0;
}

#ifdef EMULATOR_SDL_USING
int draw_vga_ch(vga_text_screen_t* vga, byte ch, bool bg_transparent, uint32 bg_color, uint32 fg_color, size_t column, size_t row) {
	if (!vga || !vga->vidmem) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw character using uninitialized vga text screen device!");

		return 1;
	}

	if (!vga->gui) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw character using non-gui vga text screen device!");

		return 1;
	}

	size_t vga_grid_width = vga->emulator_screen_width / VGA_CHAR_WIDTH;
	size_t vga_grid_height = vga->emulator_screen_height / VGA_CHAR_HEIGHT;

	size_t screen_pos = column + (row * vga_grid_width);

	if (screen_pos >= (vga_grid_width * vga_grid_height)) return 1;

	column = screen_pos % vga_grid_width;

	row = screen_pos / vga_grid_width;

	size_t font_grid_width = vga->font_width / VGA_CHAR_WIDTH;

	size_t screen_x = column * VGA_CHAR_WIDTH;
	size_t screen_y = row * VGA_CHAR_HEIGHT;

	size_t font_column = ch % font_grid_width;
	size_t font_row = ch / font_grid_width;

	for (size_t i = 0; i < VGA_CHAR_HEIGHT; i++) {
		size_t index = (font_column * VGA_CHAR_WIDTH) + (((font_row * VGA_CHAR_HEIGHT) + i) * vga->font_width);

		index /= 8;

		byte column_pixels = cur->font[index];

		for (size_t j = 0; j < VGA_CHAR_WIDTH; j++) {
			bool bit_enabled = column_pixels & (1 << j);

			size_t screen_index = screen_x + j + ((screen_y + i) * vga->emulator_screen_width);

			if (!bg_transparent) {
				vga->emulator_screen[screen_index] = bit_enabled ? fg_color : bg_color;
			}

			else if (bit_enabled) {
				vga->emulator_screen[screen_index] = fg_color;
			}
		}
	}

	return 0;
}

const uint32 vga_color_to_rgb[] = {
	[COLOR_BLACK] = 			0x000000,
	[COLOR_BLUE] = 				0x000080,
	[COLOR_GREEN] = 			0x008000,
	[COLOR_AQUA] = 				0x008080,
	[COLOR_RED] = 				0x800000,
	[COLOR_MAGENTA] = 			0x800080,
	[COLOR_BROWN] = 			0xAA5000,
	[COLOR_WHITE] = 			0xC0C0C0,
	[COLOR_BRIGHT_BLACK] = 		0x404040,
	[COLOR_BRIGHT_BLUE] = 		0x0000FF,
	[COLOR_BRIGHT_LIME] = 		0x00FF00,
	[COLOR_BRIGHT_AQUA] = 		0x00FFFF,
	[COLOR_BRIGHT_RED] = 		0xFF0000,
	[COLOR_BRIGHT_MAGENTA] = 	0xFF00FF,
	[COLOR_BRIGHT_YELLOW] = 	0xFFFF00,
	[COLOR_BRIGHT_WHITE] = 		0xFFFFFF
};

int draw_vga_text_screen_gui(vga_text_screen_t* vga) {
	if (!vga || !vga->vidmem) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw uninitialized vga text screen device!");

		return 1;
	}

	if (!vga->gui) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Tried to draw character using non-gui vga text screen device!");

		return 1;
	}

	for (size_t i = 0; i < vga->width * vga->height; i++) {
		if ((cur->mode_reg & 0x20) == 0) {
			draw_vga_ch(vga, ' ', false, 0, 0, i, 0);
		}

		uint16 ch_and_style = vga->vidmem[i];

		byte ch = ch_and_style & 0xFF;

		byte style = (ch_and_style >> 8) & 0xFF;

		byte fg_style = style & 0xF;

		byte bg_style = (style >> 4) & 0xF;

		bool bg_bright = bg_style & 0x8;
			
		if ((cur->mode_reg & 0x08) != 0) {
			bg_style = bg_style % 8;

			if (bg_bright && !blinking_text_visible) {
				draw_vga_ch(vga, ' ', false, 0, 0, i, 0); continue;
			}
		}

		if (i == vga->cursor_pos && blinking_text_visible) {
			draw_vga_ch(vga, '_', false, 0, 0xFFFFFF, i, 0);

			draw_vga_ch(vga, ch, true, vga_color_to_rgb[bg_style], vga_color_to_rgb[fg_style], i, 0);
		}

		else draw_vga_ch(vga, ch, false, vga_color_to_rgb[bg_style], vga_color_to_rgb[fg_style], i, 0);
	}

	return 0;
}
#endif

void draw_vga_text_screen_cli(vga_text_screen_t* screen) {
	if (!screen || !screen->vidmem) return;

	_ssize_t columns, rows;

	get_terminal_size(&columns, &rows);

	_size_t width = screen->width < columns ? screen->width : columns;

	_size_t height = screen->height < rows ? screen->height : rows;

	set_cursor_pos(0, 0);

	printf(default_screen_color bold white_fg);

	bool after_cursor = false;

	for (size_t i = 0; i < height; i++) {
		bool is_last_i = i == (height - 1);

		for (size_t j = 0; j < width; j++) {
			if ((cur->mode_reg & 0x20) == 0) {
				putchar(' ');

				continue;
			}

			_size_t ch_pos = j + (i * width);

			if (ch_pos == cur->cursor_pos &&
				blinking_text_visible) {
				printf(underline); after_cursor = true;
			}

			uint16 ch_and_style = screen->vidmem[ch_pos];

			byte style = (ch_and_style >> 8) & 0xFF;

			byte style_bg = (style >> 4) & 0xF;
			
			bool bg_bright = style_bg & 8;
			
			bool text_visible = ((cur->mode_reg & 0x08) == 0) || (!bg_bright || blinking_text_visible);

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

void free_vga_text_screen(vga_text_screen_t* vga) {
	if (!vga) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "VGA text screen deinitialization...");

	vga->vidmem = nullptr;

	#ifdef EMULATOR_SDL_USING
	if (!vga->gui) {
		printf(default_style);

		printf(ansi_clear_screen);
	}

	if (vga->font) {
		emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA font deinitialization...");

		free(vga->font);

		vga->font = nullptr;

		emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA font deinitialized!");
	}
	#else
	printf(default_style);

	printf(ansi_clear_screen);
	#endif

	free(vga);
	
	if (cur == vga) cur = nullptr;

	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text screen deinitialized");
}

void release_all_vga_text_screen(vga_text_screen_t* screen) {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Releasing timer updating vga screen (33 hz) and text blinking timer (2 hz)...");

	emulator_release_tick_timer(nullptr, update_cur_vga_screen);

	emulator_release_tick_timer(nullptr, text_blink_update);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Releasing VGA ports (0x3DA, 0x3C0, 0x3C1)...");

	emulator_release_port_in(0x3DA);

	emulator_release_port_out(0x3C0);

	emulator_release_port_in(0x3C1);

	free_vga_text_screen(screen);
}

void reset_vga_text_screen(vga_text_screen_t* screen) {
	if (!screen) return;

	emulator_log(false, LOG_SEVERITY_INFO, "VGA text screen reseting...");

	screen->mode_reg = 0b00001000 | 0x20;

	memset(screen->vidmem, 0, screen->width * screen->height * sizeof(screen->vidmem[0]));

	#ifdef EMULATOR_SDL_USING
	if (!screen->gui) {
		printf(default_style);

		printf(ansi_clear_screen);
	}
	#else
	printf(default_style);

	printf(ansi_clear_screen);
	#endif

	emulator_log(false, LOG_SEVERITY_INFO, "VGA text screen reseted");
}

void vga_text_screen_apply_attribute_no_gui(byte attribute) {
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
		if ((cur->mode_reg & 0x8) == 0) {
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
