#ifndef _EMULATOR_MATH_H
#define _EMULATOR_MATH_H

// Полностью заимствованно из Random OS Boosted

#include "types.h"

// Лучше использовать аналогичные функции ( они типобезопаснее, используют эти макросы для упрощения )

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(x, min_value, max_value) (MAX(MIN(x, max_value), min_value))

#define INRANGE(value, min_value, max_value) ((value) >= (min_value) && (value) <= (max_value))

#define ABS(x) ((x) < 0 ? -(x) : (x))

static inline double trunc(double x) {
	if (x != x || x == 0.0 || x == 1.0/0.0 || x == -1.0/0.0)
		return x;

	union {
		double d;
		uint64 u;
	} converter;

	converter.d = x;

	uint64 sign_bit = converter.u >> 63;
	uint64 exponent = (converter.u >> 52) & 0x7FF;
	uint64 mantissa = converter.u & 0xFFFFFFFFFFFFF;

	int exp_bias = 1023;
	int real_exp = exponent - exp_bias;

	if (real_exp < 0)
		return 0.0;

	if (real_exp >= 52)
		return x;

	uint64 mask = (1ULL << (52 - real_exp)) - 1;

	mantissa &= ~mask;

	converter.u = (sign_bit << 63) | ((uint64)exponent << 52) | mantissa;

	return converter.d;
}

static inline float ftrunc(float x) {
	if (x != x)						return x;	// NaN
	if (x == 0.0f || x == -0.0f)	return x;	// ±0
	if (x == 1.0f / 0.0f)			return x;	// +∞
	if (x == -1.0f / 0.0f)			return x;	// -∞

	union {
		float f;
		uint32 u;
	} converter;
	converter.f = x;

	uint32 sign_bit = converter.u >> 31;
	uint32 exponent = (converter.u >> 23) & 0xFF;
	uint32 mantissa = converter.u & 0x7FFFFF;

	int exp_bias = 127;
	int real_exp = (int)exponent - exp_bias;

	if (real_exp < 0) {
		return 0.0f * x;
	}

	if (real_exp >= 23) {
		return x;
	}

	int bits_to_keep = real_exp + 1;

	int bits_to_zero = 23 - bits_to_keep;

	if (bits_to_zero > 0) {
		uint32 mask = (1U << bits_to_zero) - 1;

		mantissa &= ~mask;
	}

	else mantissa = 0;

	converter.u = (sign_bit << 31) | (exponent << 23) | mantissa;

	return converter.f;
}

#define MOD(a, b) ((a) - (trunc((a) / (b)) * (b)))

#define FMOD(a, b) ((a) - (ftrunc((a) / (b)) * (b)))

static inline bool isPowerOfTwoU32(_size_t x) {
	return x > 0 && ((x & (x - 1)) == 0);
}

_size_t align_down(_size_t x, _size_t align);

_size_t align_up(_size_t x, _size_t align);

#endif
