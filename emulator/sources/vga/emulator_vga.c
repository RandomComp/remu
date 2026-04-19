#include "vga/emulator_vga.h"

#include "types.h"

#include "colors.h"

#include "ansi.h"

#include "utils.h"

#include "main.h"

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

vga_text_screen_t* cur = nullptr;

bool reg1_trigger = false;

byte reg = 0, display_vidmem = 0;

static _size_t read_input_status_reg1() {
	if (!cur) return 0;

	reg1_trigger = true;

	return 0;
}

static void write_attrib_addr_or_data_reg(_size_t data) {
	if (!cur) return;

	if (reg1_trigger) {
		reg = data & 0x1F;

		display_vidmem |= data & 0x20;

		reg1_trigger = false;
	}

	else if (reg & 0x10) {
		cur->mode_reg = data & (~0x20);

		display_vidmem |= data & 0x20;

		if (display_vidmem != 0)
			cur->mode_reg |= 0x20;
		else
			cur->mode_reg &= ~0x20;

		display_vidmem = 0;
		
		reg = 0;
	}
}

static _size_t read_attrib_data_reg() {
	if (!cur) return 0;

	if (!reg1_trigger && reg == 0x10) {
		reg = 0;

		return cur->mode_reg;
	}

	reg1_trigger = false;

	display_vidmem = 0;

	return 0;
}

vga_text_screen_t* init_vga_text_screen(_ssize_t columns, _ssize_t rows) {
	vga_text_screen_t* screen = malloc(sizeof(vga_text_screen_t));

	memset(screen, 0, sizeof(vga_text_screen_t));

	screen->vidmem = (uint16*)malloc(columns * rows * sizeof(uint16));

	screen->width = columns; screen->height = rows;

	screen->mode_reg = 0b00001000 | 0x20;

	clear_vga_text_screen(screen);

	cur = screen;

	setup_port_in(0x3DA, read_input_status_reg1);

	setup_port_out(0x3C0, write_attrib_addr_or_data_reg);

	setup_port_in(0x3C1, read_attrib_data_reg);

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

	_ssize_t term_size = columns * rows;

	_size_t screen_size = screen->width * screen->height;

	_size_t size = screen_size < term_size ? screen_size : term_size;

	set_cursor_pos(0, 0);

	printf(default_screen_color);

	for (size_t i = 0; i < size; i++) {
		if ((cur->mode_reg & 0x20) == 0) {
			putchar(' ');

			continue;
		}

		byte c = screen->vidmem[i] & 0xFF;

		byte style = (screen->vidmem[i] >> 8) & 0xFF;

		vga_text_screen_apply_attribute(style);

		printf("%s", cp437[c]);
	}

	printf(default_screen_color);

	// fflush(stdout);
}

void free_vga_text_screen(vga_text_screen_t* screen) {
	if (!screen) return;

	if (screen->vidmem) free(screen->vidmem);

	screen->vidmem = nullptr;

	free(screen);

	screen = nullptr;
}

void vga_text_screen_apply_attribute(byte attribute) {
	byte style_fg = attribute & 0xF;

	byte style_bg = (attribute >> 4) & 0xF;

	byte ansi_code_fg = color_to_ansi(style_fg % 8);

	byte ansi_code_bg = color_to_ansi(style_bg % 8);
	
	bool fg_bright = style_fg & 8;
	
	bool bg_bright = style_bg & 8;

	if (fg_bright) {
		ansi_code_fg += 60;
	}

	if (bg_bright) {
		if ((cur->mode_reg & 0x8) != 0) {
			printf(blink);
		}

		else {
			ansi_code_bg += 60;
		}
	}

	// if (bg_bright && (cur->mode_reg & 0x8) != 0) {
	// 	printf(blink);
	// }

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

	// else {
	// 	printf("\x1B[%im", (int)ansi_code_bg + 10);
	// }
}