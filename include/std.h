#ifndef _EMULATOR_OS_STD_H
#define _EMULATOR_OS_STD_H

#include "types.h"

void init_std(uint16* _vidmem);

void clear_screen(byte style);

void putch(byte c);

void kprint(const c_str str);

void kprintf(const c_str format, ...);

_size_t get_num_digits(_ssize_t num, _size_t base);

bool _isalpha(char c);

bool _isupper(char c);

bool _islower(char c);

char upper(char c);

char lower(char c);

ErrorCode parse_hex(byte* result, _size_t res_size, const c_str str);

_uintmax_t parse_num(const c_str str, _uintmax_t radix);

void print_hex(byte* num, _size_t size);

void print_num(_size_t num, _size_t base);

void nblk__set_cursor_pos(_ssize_t x, _ssize_t y);

void get_cursor_pos(_ssize_t* x, _ssize_t* y);

void set_style(byte style);

byte get_style();

#define set_cursor_pos(x, y) nblk__set_cursor_pos(x, y)

#endif
