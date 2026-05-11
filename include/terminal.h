#ifndef _EMULATOR_OS_TERMINAL_H
#define _EMULATOR_OS_TERMINAL_H

#include "types.h"

typedef struct terminal_out_t {
	void (*putch)(byte style, byte c);

	void (*set_cur_pos)(ssize_t column, ssize_t row, bool view);
	void (*get_cur_pos)(ssize_t* column, ssize_t* row);

	void (*enable_view_cursor)(void);
	void (*disable_view_cursor)(void);

	ssize_t columns, rows;
} terminal_out_t;

typedef struct terminal_in_t {
	byte (*getch)(void); // unblocking getch
} terminal_in_t;

#endif
