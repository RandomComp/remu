#ifndef _EMULATOR_OS_TERMINAL_H
#define _EMULATOR_OS_TERMINAL_H

#include "types.h"

typedef struct terminal_out_t {
	void (*setch)(size_t column, size_t row, byte style, byte c);

	byte (*getch)(size_t column, size_t row);

	byte (*get_style)(size_t column, size_t row);

	ssize_t columns, rows;
} terminal_out_t;

typedef struct terminal_in_t {
	byte (*getch)(void); // unblocking getch
} terminal_in_t;

#endif
