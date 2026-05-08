#ifndef _EMULATOR_OS_STD_H
#define _EMULATOR_OS_STD_H

#include "types.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "math/math.h"

#include "terminal.h"

typedef enum parse_num_err_e {
	PARSE_NUM_OK = 0,
	PARSE_NUM_INVLD_STR,
	PARSE_NUM_INVLD_RADIX,
	PARSE_NUM_INVLD_NUM,
} parse_num_err_e;

void init_std(terminal_out_t stdout, terminal_in_t stdin);

byte getch(void);

byte blkgetch(void);

size_t getstr(bool show, byte* buf, size_t buf_size);

byte* getstr_hist(bool show, size_t* _inputed_size, byte history[][64], ssize_t command_index, size_t history_size);

size_t getstr_hist_with_auto_add_on(bool show, byte* buf, size_t buf_size, byte history[][64], ssize_t command_index, size_t history_len);

void set_style(byte style);
byte get_style(void);

byte get_style_from(size_t column, size_t row);
byte getch_from(size_t column, size_t row);

size_t get_columns(void);
size_t get_rows(void);

void setch(size_t column, size_t row, byte style, byte c);

void putch(byte c);

void clear_screen(byte style);

void clear_line();

size_t kprint(const c_str str);
size_t sprint(byte *s, const c_str str);

ssize_t vsnprintf(byte* s, ssize_t max_len, const c_str format, va_list list);

ssize_t snprintf(byte* s, ssize_t max_len, const c_str format, ...);

ssize_t vsprintf(byte* s, const c_str format, va_list list);

ssize_t sprintf(byte* s, const c_str format, ...);

size_t kprintf(const c_str format, ...);

size_t sscanf(const byte* s, const c_str format, ...);

uintmax_t get_num_digits(uintmax_t num, uintmax_t base, bool signable);

bool isnum(byte c);
bool isalnum(byte c);
bool isalpha(byte c);
bool isascii(byte c);
bool isprintable(byte c);

bool isupper(byte c);
bool islower(byte c);

byte upper(byte c);
byte lower(byte c);

byte* strtok(byte* str, const byte* delim);
byte* parse_cli_args(byte* _str);
void strip_str(byte* str, size_t size);

ErrorCode parse_hex(byte* result, size_t res_size, const c_str str);

int parse_num(const c_str str, uintmax_t radix, uintmax_t* result, size_t* len);

int nparse_num(const c_str str, ssize_t max_size, uintmax_t radix, uintmax_t* result, size_t* len);

void snprint_hex(byte* s, ssize_t max_size, byte* num, size_t size);
void print_hex(byte* num, size_t offset, size_t size);

size_t snprint_num(byte* s, ssize_t max_size, size_t num, size_t radix, bool num_signed, bool always_show_sign);
size_t print_num(uintmax_t num, uintmax_t radix, bool num_signed, bool always_show_sign);

void set_cursor_pos(ssize_t x, ssize_t y);

void get_cursor_pos(ssize_t* x, ssize_t* y);

#endif
