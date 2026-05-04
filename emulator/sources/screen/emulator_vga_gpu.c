#include "screen/emulator_vga_gpu.h"

#include "screen/emulator_vga_fwd.h"

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

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(x, min_value, max_value) (MAX(MIN(x, max_value), min_value))

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
	"═", "╬", "╧", "╨", "╤", 
	"╥", "╙", "╘", "╒", "╓", "╫", 
	"╪", "┘", "┌", "█", "▄", "▌", 
	"▐", "▀", "α", "ß", "Γ", "π", 
	"Σ", "σ", "µ", "τ", "Φ", "Θ", 
	"Ω", "δ", "∞", "φ", "ε", "∩", 
	"≡", "±", "≥", "≤", "⌠", "⌡", 
	"÷", "≈", "°", "∙", "·", "√", 
	"ⁿ", "²", "■", " "
};

static vga_text_device_t* cur = nullptr;

static bool attrib_flip_trigger_flag = false;

static byte attrib_reg = 0, display_vidmem = 0;

static _size_t attrib_flip_trigger() {
	if (!cur) return 0;

	attrib_flip_trigger_flag = true;

	emulator_log(false, LOG_SEVERITY_TRACE, "Write addr/data flag triggered (0x3DA) for 0x3C0 in VGA text screen");

	return 0;
}

static void attrib_reg_write(_size_t data) {
	if (!cur) return;

	if (attrib_flip_trigger_flag) {
		attrib_reg = data & 0x1F;

		display_vidmem |= data & 0x20;

		attrib_flip_trigger_flag = false;

		emulator_log(false, LOG_SEVERITY_TRACE, "Writed %llx as register to 0x3C0 (VGA text screen)\r", attrib_reg);
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

		emulator_log(false, LOG_SEVERITY_TRACE, "Writed %llx as data to 0x3C0 (VGA text screen)\r", data);
	}
}

static _size_t attrib_reg_read() {
	if (!cur) return 0;

	if (!attrib_flip_trigger_flag && attrib_reg == 0x10) {
		attrib_reg = 0;

		emulator_log(false, LOG_SEVERITY_TRACE, "VGA text screen mode register readed (0x3C1)");

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

vga_text_device_t* init_vga_text_device(ram_t* ram, _ssize_t columns, _ssize_t rows) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text device initialization...");

	size_t vidmem_size = columns * rows * sizeof(uint16);

	uint64 vidmem_ram_addr = 0xB8000;

	if (!ram || !ram->mem_ptr) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize VGA text video memory because RAM is not initialized!"); 
		
		return nullptr;
	}

	if (ram->mem_size < (vidmem_ram_addr + vidmem_size)) {
		emulator_log(true, LOG_SEVERITY_ERROR, "Cannot initialize VGA text video memory because RAM is lower than vidmem address end %llx!", vidmem_ram_addr + vidmem_size); 
		
		return nullptr;
	}

	vga_text_device_t* vga = malloc(sizeof(vga_text_device_t));

	memset(vga, 0, sizeof(vga_text_device_t));

	vga->vidmem_ram_addr = vidmem_ram_addr;

	vga->vidmem = (uint16*)(ram->mem_ptr + vga->vidmem_ram_addr);

	vga->width = columns; vga->height = rows; vga->bpp = 2 * 8;

	vga->cursor_pos = 0;

	vga->mode_reg = 0b00001000 | 0x20;

	vga->crt_reg_a = 0b00000000;
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "VGA text screen mode register (no video mode): 0b%08b\r", vga->mode_reg);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Clearing VGA text screen...");

	clear_vga_text_screen(vga);

	cur = vga;
	
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
	
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text device initialized");

	return vga;
}

void clear_vga_text_screen(vga_text_device_t* screen) {
	if (!screen || !screen->vidmem) return;

	for (size_t i = 0; i < screen->width * screen->height; i++) {
		screen->vidmem[i] = ' ' | (0x0F << 8);
	}
}

int draw_vga_text(vga_text_device_t* vga, const c_str text, byte style, _size_t column, _size_t row) {
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

void free_vga_text_device(vga_text_device_t* vga) {
	if (!vga) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "VGA text screen deinitialization...");

	vga->vidmem = nullptr;

	free(vga);
	
	if (cur == vga) cur = nullptr;

	emulator_log(true, LOG_SEVERITY_VERBOSE, "VGA text screen deinitialized");
}

void release_all_vga_text_device(vga_text_device_t* vga_device) {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Releasing VGA GPU ports (0x3DA, 0x3C0, 0x3C1)...");

	emulator_release_port_in(0x3DA);

	emulator_release_port_out(0x3C0);

	emulator_release_port_in(0x3C1);

	free_vga_text_device(vga_device);
}

void reset_vga_text_device(vga_text_device_t* device) {
	if (!device) return;

	emulator_log(false, LOG_SEVERITY_INFO, "VGA GPU reseting...");

	device->mode_reg = 0b00001000 | 0x20;

	memset(device->vidmem, 0, device->width * device->height * sizeof(device->vidmem[0]));

	emulator_log(false, LOG_SEVERITY_INFO, "VGA GPU reseted");
}
