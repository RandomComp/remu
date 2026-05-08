#ifndef _EMULATOR_OS_VGA_H
#define _EMULATOR_OS_VGA_H

#include "types.h"

#include "terminal.h"

#define VGA_COLUMNS 80

#define VGA_ROWS 25

#define VGA_VIDMEM_SIZE (VGA_COLUMNS * VGA_ROWS)

void init_vga(void);

terminal_out_t init_vga_stdout(void);

byte vga_getch(size_t column, size_t row);
byte vga_get_style(size_t column, size_t row);
void vga_setch(size_t column, size_t row, byte style, byte c);

void crt_set_cursor_pos(size_t x, size_t y);

void crt_enable_cursor(void);

void crt_disable_cursor(void);

byte read_mode_reg(void);

void enable_blink(void);

void disable_blink(void);

#endif
