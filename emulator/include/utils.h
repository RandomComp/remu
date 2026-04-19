#ifndef _EMULATOR_UTILS_H
#define _EMULATOR_UTILS_H

#include "types.h"

void set_cursor_pos(_ssize_t col, _ssize_t row);

void get_terminal_size(_ssize_t* columns, _ssize_t* rows);

void change_title(const char* title);

#ifdef IS_UNIX
#include <termios.h>

#include <time.h>

void switch_to_default(struct termios* orig_termios);

struct termios* switch_to_raw();

void init_timer(timer_t* timer, time_t ns, void (*handler)(int));

void setup_timer(timer_t timerid, time_t ns);
#endif

#endif