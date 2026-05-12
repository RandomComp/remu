#ifndef _EMULATOR_OS_STD_H
#define _EMULATOR_OS_STD_H

#include "types.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "math/math.h"

#include "std/stdio.h"
#include "std/string.h"

#include "terminal.h"

typedef enum parse_num_err_e {
	PARSE_NUM_OK = 0,
	PARSE_NUM_INVLD_STR,
	PARSE_NUM_INVLD_RADIX,
	PARSE_NUM_INVLD_NUM,
} parse_num_err_e;

void init_std(terminal_out_t stdout, terminal_in_t stdin);

void set_style(byte style);
byte get_style(void);

size_t get_columns(void);
size_t get_rows(void);

void clear_screen(byte style);

void clear_line(void);

uintmax_t get_num_digits(uintmax_t num, uintmax_t base, bool signable);

void set_cursor_pos(ssize_t x, ssize_t y, bool view);
void get_cursor_pos(ssize_t* x, ssize_t* y);

#endif
