#ifndef _EMULATOR_OS_STD_H
#define _EMULATOR_OS_STD_H

#include "types.h"

#define STD_COLUMNS 80

#define STD_ROWS 25

void init_std(uint16* _vidmem);

void clear_screen(byte style);

void putch(byte c);

void clear_line();

void kprint(const c_str str);

size_t vsprintf(char* s, const c_str format, va_list list);

void kprintf(const c_str format, ...);

size_t get_num_digits(ssize_t num, size_t base);

bool isalpha(byte c);
bool isascii(byte c);
bool isprintable(byte c);

bool isupper(byte c);
bool islower(byte c);

byte upper(byte c);
byte lower(byte c);

char* strtok(char* str, const char* delim);

ErrorCode parse_hex(byte* result, size_t res_size, const c_str str);

uintmax_t parse_num(const c_str str, uintmax_t radix);

void sprint_hex(char* s, byte* num, size_t size);
void print_hex(byte* num, size_t size);

size_t snprint_num(char* s, ssize_t size, size_t num, size_t base, bool num_signed, bool always_show_sign);
void print_num(size_t num, size_t base, bool num_signed);

void set_cursor_pos(ssize_t x, ssize_t y);

void get_cursor_pos(ssize_t* x, ssize_t* y);

void set_style(byte style);

byte get_style();

#endif
