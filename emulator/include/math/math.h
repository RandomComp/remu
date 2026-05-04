#ifndef _EMULATOR_MATH_H
#define _EMULATOR_MATH_H

#include "types.h"

// Лучше использовать аналогичные функции ( они типобезопаснее, используют эти макросы для упрощения )

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(x, min_value, max_value) (MAX(MIN(x, max_value), min_value))

#define INRANGE(value, min_value, max_value) ((value) >= (min_value) && (value) <= (max_value))

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define MOD(a, b) ((a) - (trunc((a) / (b)) * (b)))

#define FMOD(a, b) ((a) - (ftrunc((a) / (b)) * (b)))

static inline bool isPowerOfTwoU32(_size_t x) {
	return x > 0 && ((x & (x - 1)) == 0);
}

_size_t align_down(_size_t x, _size_t align);

_size_t align_up(_size_t x, _size_t align);

#endif
