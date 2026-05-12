#ifndef _EMULATOR_OS_STD_STRING_H
#define _EMULATOR_OS_STD_STRING_H

#include "types.h"

#include "builtins/string.h"

byte* strtok(byte* str, const byte* delim);
byte* parse_cli_args(byte* _str);
void strip_str(byte* str, size_t size);

bool isnum(byte c);
bool isalnum(byte c);
bool isalpha(byte c);
bool isascii(byte c);
bool isprintable(byte c);

bool isupper(byte c);
bool islower(byte c);

byte upper(byte c);
byte lower(byte c);

#endif
