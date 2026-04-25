#ifndef _EMULATOR_OS_VGA_H
#define _EMULATOR_OS_VGA_H

#include "types.h"

#define columns 80

#define rows 25

#define vidmem_size (columns * rows)

void crt_set_cursor_pos(_size_t x, _size_t y);

void crt_enable_cursor();

void crt_disable_cursor();

byte read_mode_reg();

void enable_blink();

void disable_blink();

#endif
