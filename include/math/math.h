#ifndef _EMULATOR_OS_MATH_H
#define _EMULATOR_OS_MATH_H

// Полностью заимствованно из Random OS Boosted

#include "types.h"

// Макросы

#define PI 3.1415926535

#define E 2.718281828459

#define INFINITY (__builtin_inff())

// Лучше использовать аналогичные функции ( они типобезопаснее, используют эти макросы для упрощения )

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(x, min_value, max_value) (MAX(MIN(x, max_value), min_value))

#define INRANGE(value, min_value, max_value) ((value) >= (min_value) && (value) <= (max_value))

#define ABS(x) ((x) < 0 ? -(x) : (x))

typedef struct ProcessorMathState {
	bool bOverflow; // Флаг переполнения типа
	_size_t divRem; // Остаток от деления, вычисленный при вычислении деления
} ProcessorMathState;

bool getOverflowFlag();

int32 pow32(int32 a, int32 b);
int64 pow64(int64 a, int64 b);

_size_t powU32(_size_t a, _size_t b);
uint64 powU64(uint64 a, uint64 b);

static inline _size_t minU32(_size_t a, _size_t b) {
	return MIN(a, b);
}

static inline uint64 minU64(uint64 a, uint64 b) {
	return MIN(a, b);
}

static inline int32 min32(int32 a, int32 b) {
	return MIN(a, b);
}

static inline int64 min64(int64 a, int64 b) {
	return MIN(a, b);
}

static inline float nblk__fmin(float a, float b) {
	return MIN(a, b);
}

static inline double min(double a, double b) {
	return MIN(a, b);
}

static inline _size_t maxU32(_size_t a, _size_t b) {
	return MAX(a, b);
}

static inline uint64 maxU64(uint64 a, uint64 b) {
	return MAX(a, b);
}

static inline int32 max32(int32 a, int32 b) {
	return MAX(a, b);
}

static inline int64 max64(int64 a, int64 b) {
	return MAX(a, b);
}

static inline float nblk__fmax(float a, float b) {
	return MAX(a, b);
}

static inline double max(double a, double b) {
	return MAX(a, b);
}

static inline _size_t clampU32(_size_t x, _size_t min, _size_t max) {
	return CLAMP(x, min, max);
}

static inline uint64 clampU64(uint64 x, uint64 min, uint64 max) {
	return CLAMP(x, min, max);
}

static inline int32 clamp32(int32 x, int32 min, int32 max) {
	return CLAMP(x, min, max);
}

static inline int64 clamp64(int64 x, int64 min, int64 max) {
	return CLAMP(x, min, max);
}

static inline float fclamp(float x, float min, float max) {
	return CLAMP(x, min, max);
}

static inline double clamp(double x, double min, double max) {
	return CLAMP(x, min, max);
}

static inline bool inRangeU8(uint8 x, uint8 min, uint8 max) {
	return INRANGE(x, min, max);
}

static inline bool inRangeU16(uint16 x, uint16 min, uint16 max) {
	return INRANGE(x, min, max);
}

static inline bool inRangeU32(_size_t x, _size_t min, _size_t max) {
	return INRANGE(x, min, max);
}

static inline bool inRangeU64(uint64 x, uint64 min, uint64 max) {
	return INRANGE(x, min, max);
}

static inline bool inRange8(int8 x, int8 min, int8 max) {
	return INRANGE(x, min, max);
}

static inline bool inRange16(int16 x, int16 min, int16 max) {
	return INRANGE(x, min, max);
}

static inline bool inRange32(int32 x, int32 min, int32 max) {
	return INRANGE(x, min, max);
}

static inline bool inRange64(int64 x, int64 min, int64 max) {
	return INRANGE(x, min, max);
}

static inline bool finRange(float x, float min, float max) {
	return INRANGE(x, min, max);
}

static inline bool inRange(double x, double min, double max) {
	return INRANGE(x, min, max);
}

static inline double trunc(double x) {
	return x;
}

static inline float ftrunc(float x) {
	return x;
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

float nblk__fmod(float a, float b);

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

static inline bool isPowerOfTwoU32(_size_t x) {
	return x > 0 && (x & (x - 1) == 0);
}

_size_t align_down(_size_t x, _size_t align);

_size_t align_up(_size_t x, _size_t align);

static inline double nblk__floor(double x) {
	double result = trunc(x);

	if (x < 0 && (frac(x) != 0))
		result -= 1;

	return result;
}

static inline double nblk__ceil(double x) {
	double result = trunc(x);

	if (x > 0 && (frac(x) != 0))
		result += 1;

	return result;
}

static inline double round(double x) {
	if (x < 0)
		return  frac(x) >= 0.5 ? nblk__floor(x) : nblk__ceil(x);

	return      frac(x) >= 0.5 ? nblk__ceil(x) : nblk__floor(x);
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

#define fmin nblk__fmin
#define fmax nblk__fmax
#define fmod nblk__fmod
#define ceil nblk__ceil
#define floor nblk__floor