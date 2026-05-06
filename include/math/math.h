#ifndef _EMULATOR_OS_MATH_H
#define _EMULATOR_OS_MATH_H

// Полностью заимствованно из Random OS Boosted

#include "types.h"

// Макросы

#define PI 3.1415926535

#define E 2.718281828459

#define INFINITY (__builtin_inff())

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(x, min_value, max_value) (MAX(MIN(x, max_value), min_value))

#define INRANGE(value, min_value, max_value) ((value) >= (min_value) && (value) <= (max_value))

#define ABS(x) ((x) < 0 ? -(x) : (x))

// Лучше использовать аналогичные функции ( они типобезопаснее, используют эти макросы для упрощения )

typedef struct ProcessorMathState {
	bool bOverflow; // Флаг переполнения типа
	size_t divRem; // Остаток от деления, вычисленный при вычислении деления
} ProcessorMathState;

bool getOverflowFlag();

int32 pow32(int32 a, int32 b);
int64 pow64(int64 a, int64 b);

size_t powU32(size_t a, size_t b);
uint64 powU64(uint64 a, uint64 b);

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

#define GET_FIRST_DIGIT(x) (uint8)(MOD(ABS(x), 10))

static inline double frac(double x) {
	return x - trunc(x);
}

static inline float ffrac(float x) {
	return x - trunc(x);
}

static inline uint8 fgetFirstNumberAfterDecimalPoint(float x) {
	return (uint8)(ABS(ffrac(x)) * 10.0f) % 10;
}

static inline uint8 getFirstNumberAfterDecimalPoint(double x) {
	return (uint8)(ABS(frac(x)) * 10.0) % 10;
}

double mod(double a, double b);

float _fmod(float a, float b);

uint8 fgetCountDecimalPlaces(float x);

uint16 getCountDecimalPlaces(double x);

static inline int32 fscaleToInteger(float x) {
	if (x == 0.0f) return 0;

	return x * pow64(10, fgetCountDecimalPlaces(x));
}

static inline int64 scaleToInteger(double x) {
	if (x == 0.0f) return 0;

	return x * pow64(10, getCountDecimalPlaces(x));
}

static inline bool isPowerOfTwoU32(size_t x) {
	return x > 0 && ((x & (x - 1)) == 0);
}

intmax_t align_down(intmax_t x, intmax_t align);

intmax_t align_up(intmax_t x, intmax_t align);

static inline double floor(double x) {
	double result = trunc(x);

	if (x < 0 && (frac(x) != 0))
		result -= 1;

	return result;
}

static inline double ceil(double x) {
	double result = trunc(x);

	if (x > 0 && (frac(x) != 0))
		result += 1;

	return result;
}

static inline double round(double x) {
	if (x < 0)
		return  frac(x) >= 0.5 ? floor(x) : ceil(x);

	return      frac(x) >= 0.5 ? ceil(x) : floor(x);
}

static inline float ffloor(float x) {
	float result = ftrunc(x);

	if (x < 0 && (ffrac(x) != 0))
		result -= 1;

	return result;
}

static inline float fceil(float x) {
	float result = ftrunc(x);

	if (x > 0 && (ffrac(x) != 0))
		result += 1;

	return result;
}

static inline float fround(float x) {
	if (x < 0)
		return  ffrac(x) >= 0.5f ? ffloor(x) : fceil(x);

	return      ffrac(x) >= 0.5f ? fceil(x) : ffloor(x);
}

#endif
