#include "vga/emulator_vga.h"

#include "types.h"

#include "colors.h"

#include "ansi.h"

#include "utils.h"

#include "main.h"

#include "emulator.h"

#include "emulator.h"

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

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

	emulator_log(false, LOG_SEVERITY_INFO, "Write addr/data flag triggered (0x3DA) for 0x3C0 in VGA text screen\n");

	return 0;
}

static void attrib_reg_write(_size_t data) {
	if (!cur) return;

	if (attrib_flip_trigger_flag) {
		attrib_reg = data & 0x1F;

		display_vidmem |= data & 0x20;

		attrib_flip_trigger_flag = false;

		emulator_log(false, LOG_SEVERITY_INFO, "Writed %llx as register to 0x3C0 (VGA text screen)\n\r", attrib_reg);
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

		emulator_log(false, LOG_SEVERITY_INFO, "Writed %llx as data to 0x3C0 (VGA text screen)\n\r", data);
	}
}

static _size_t attrib_reg_read() {
	if (!cur) return 0;

	if (!attrib_flip_trigger_flag && attrib_reg == 0x10) {
		attrib_reg = 0;

		emulator_log(false, LOG_SEVERITY_INFO, "VGA text screen mode register readed (0x3C1)\n");

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
	if (!cur) return;

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

void update_cur_vga_screen() {
	if (!cur) return;

	draw_vga_text_screen(cur);
}

static bool blinking_text_visible = false;

void text_blink_update() {
	blinking_text_visible = !blinking_text_visible;
}

vga_text_screen_t* init_vga_text_screen(ram_t* ram, _ssize_t columns, _ssize_t rows) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text screen initialization...\n");

	size_t vidmem_size = columns * rows * sizeof(uint16);

	if (!ram || !ram->mem_ptr) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize VGA because RAM is not initialized!"); 
		
		return nullptr;
	}

	if (ram->mem_size < (0xB8000 + vidmem_size)) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize VGA because RAM is lower than vidmem address end %llx!", 0xB8000 + vidmem_size); 
		
		return nullptr;
	}

	vga_text_screen_t* screen = malloc(sizeof(vga_text_screen_t));

	memset(screen, 0, sizeof(vga_text_screen_t));

	screen->vidmem = (uint16*)(ram->mem_ptr + 0xB8000);

	screen->vidmem_ram_addr = 0xB8000;

	screen->width = columns; screen->height = rows; screen->bpp = 2 * 8;

	screen->cursor_pos = 0;

	screen->mode_reg = 0b00001000 | 0x20;

	screen->crt_reg_a = 0b00000000;
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "VGA text screen mode register (no video mode): 0b%08b\n\r", screen->mode_reg);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Clearing VGA text screen...\n");

	clear_vga_text_screen(screen);

	cur = screen;
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up VGA screen update timer (33 hz) and text blinking timer (2 hz)...\n");

	emulator_setup_tick_timer(nullptr, update_cur_vga_screen, 33);
	emulator_setup_tick_timer(nullptr, text_blink_update, 500);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up ports (0x3DA, 0x3C0, 0x3C1) for VGA...\n");

	emulator_setup_port_in(0x3DA, attrib_flip_trigger);
	emulator_setup_port_out(0x3C0, attrib_reg_write);
	emulator_setup_port_in(0x3C1, attrib_reg_read);

	emulator_setup_port_out(0x3D4, crt_select_reg);
	emulator_setup_port_out(0x3D5, crt_write_reg);
	emulator_setup_port_in(0x3D5, crt_read_reg);
	
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text screen initialized\n");

	printf(ansi_clear_screen);

	return screen;
}

void clear_vga_text_screen(vga_text_screen_t* screen) {
	if (!screen || !screen->vidmem) return;

	for (size_t i = 0; i < screen->width * screen->height; i++) {
		screen->vidmem[i] = ' ' | (0x0F << 8);
	}
}


void draw_vga_text_screen(vga_text_screen_t* screen) {
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

void free_vga_text_screen(vga_text_screen_t* screen) {
	if (!screen) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "VGA text screen deinitialization...\n");

	screen->vidmem = nullptr;

	free(screen);
	
	if (cur == screen) cur = nullptr;

	printf(default_style);

	printf(ansi_clear_screen);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text screen deinitialized\n");
}

void release_all_vga_text_screen(vga_text_screen_t* screen) {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Releasing timer updating vga screen (33 hz) and text blinking timer (2 hz)...\n");

	emulator_release_tick_timer(nullptr, &update_cur_vga_screen);

	emulator_release_tick_timer(nullptr, &text_blink_update);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Releasing VGA ports (0x3DA, 0x3C0, 0x3C1)...\n");

	emulator_release_port_in(0x3DA);

	emulator_release_port_out(0x3C0);

	emulator_release_port_in(0x3C1);

	free_vga_text_screen(screen);
}

void reset_vga_text_screen(vga_text_screen_t* screen) {
	if (!screen) return;

	emulator_log(false, LOG_SEVERITY_INFO, "VGA text screen reseting...\n");

	screen->mode_reg = 0b00001000 | 0x20;

	memset(screen->vidmem, 0, screen->width * screen->height * sizeof(screen->vidmem[0]));

	printf(default_style);

	printf(ansi_clear_screen);

	emulator_log(false, LOG_SEVERITY_INFO, "VGA text screen reseted\n");
}

// void vga_text_screen_apply_attribute(byte attribute) {
// 	byte style_fg = attribute & 0xF;

// 	byte style_bg = (attribute >> 4) & 0xF;
	
// 	bool bg_bright = style_bg & 8;

// 	if (bg_bright && (cur->mode_reg & 0x8) != 0) {
// 		printf(blink);
// 	}

// 	if (style_fg == COLOR_BRIGHT_WHITE) {
// 		printf(bold white_fg);
// 	}

// 	if (style_fg == COLOR_BLACK) {
// 		printf(bright_black_fg);
// 	}

// 	else if (style_fg == COLOR_BRIGHT_BLACK) {
// 		printf(bold gray_fg);
// 	}
	
// 	else {
// 		printf("\x1B[38;5;%im", (int)style_fg);
// 	}

// 	if (style_bg == COLOR_BLACK) {
// 		printf(bright_black_bg);
// 	}

// 	else if (style_bg == COLOR_BRIGHT_BLACK) {
// 		printf(bold gray_bg);
// 	}

// 	else {
// 		printf("\x1B[48;5;%im", (int)style_bg);
// 	}
// }

void vga_text_screen_apply_attribute(byte attribute) {
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