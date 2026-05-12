#ifndef _EMULATOR_OS_STD_IO_H
#define _EMULATOR_OS_STD_IO_H

#include "types.h"

byte getch(void);

byte blkgetch(void);

size_t getstr(bool show, byte* buf, size_t buf_size);

byte* getstr_hist(bool show, size_t* _inputed_size, byte history[][64], ssize_t command_index, size_t history_size);

void putch(byte c);

size_t kprint(const byte* str);
size_t sprint(byte *s, const byte* str);

ssize_t vsnprintf(byte* s, ssize_t max_len, const byte* format, va_list list);
ssize_t snprintf(byte* s, ssize_t max_len, const byte* format, ...);
ssize_t vsprintf(byte* s, const byte* format, va_list list);
ssize_t sprintf(byte* s, const byte* format, ...);

size_t kprintf(const byte* format, ...);

size_t sscanf(const byte* s, const byte* format, ...);

ErrorCode parse_hex(byte* result, size_t res_size, const byte* str);

int parse_num(const byte* str, uintmax_t radix, uintmax_t* result, size_t* len);
int nparse_num(const byte* str, ssize_t max_size, uintmax_t radix, uintmax_t* result, size_t* len);

void snprint_hex(byte* s, ssize_t max_size, byte* num, size_t size);

size_t snprint_num(byte* s, ssize_t max_size, size_t num, size_t radix, bool num_signed, bool always_show_sign);
size_t print_num(uintmax_t num, uintmax_t radix, bool num_signed, bool always_show_sign);

#endif
