#ifndef _EMULATOR_OS_STD_H
#define _EMULATOR_OS_STD_H

#include "types.h"

void init_std(uint16* _vidmem);

void clear_screen(byte style);

void putch(byte c);

void kprint(const c_str str);

void kprintf(const c_str format, ...);

size_t get_num_digits(ssize_t num, size_t base);

bool _isalpha(char c);

bool _isupper(char c);

bool _islower(char c);

char upper(char c);

char lower(char c);

ErrorCode parse_hex(byte* result, size_t res_size, const c_str str);

uintmax_t parse_num(const c_str str, uintmax_t radix);

void print_hex(byte* num, size_t size);

void print_num(size_t num, size_t base);

void nblk__set_cursor_pos(ssize_t x, ssize_t y);

void get_cursor_pos(ssize_t* x, ssize_t* y);

void set_style(byte style);

byte get_style();

#define set_cursor_pos(x, y) nblk__set_cursor_pos(x, y)

#endif
