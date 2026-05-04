#ifndef _EMULATOR_OS_VGA_H
#define _EMULATOR_OS_VGA_H

#include "types.h"

#define VGA_COLUMNS 80

#define VGA_ROWS 25

#define VGA_VIDMEM_SIZE (VGA_COLUMNS * VGA_ROWS)

void crt_set_cursor_pos(size_t x, size_t y);

void crt_enable_cursor();

void crt_disable_cursor();

byte read_mode_reg();

void enable_blink();

void disable_blink();

#endif
