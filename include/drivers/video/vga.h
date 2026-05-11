#ifndef _EMULATOR_OS_VGA_H
#define _EMULATOR_OS_VGA_H

#include "types.h"

#include "terminal.h"

#define VGA_COLUMNS 80

#define VGA_ROWS 25

#define VGA_VIDMEM_SIZE (VGA_COLUMNS * VGA_ROWS)

void init_vga(void);

terminal_out_t init_vga_stdout(void);

void vga_putch(byte style, byte c);

void crt_set_cursor_pos(ssize_t x, ssize_t y);

void crt_enable_cursor(byte cursor_start, byte cursor_end);
void crt_disable_cursor(void);

byte vga_read_mode_reg(void);

void vga_enable_blink(void);
void vga_disable_blink(void);
bool vga_is_blink(void);

#endif
