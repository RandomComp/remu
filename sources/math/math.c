#include "math/math.h"

#include "types.h"

static ProcessorMathState mathState;

// TODO: Сделать реализацию вычисления синуса и косинуса

bool getOverflowFlag() {
	bool result = mathState.bOverflow;
	
	mathState.bOverflow = false;

	return result;
}

uint8 fgetCountDecimalPlaces(float x) {
	uint8 result = 0;

	while (fgetFirstNumberAfterDecimalPoint(x) != 0 && result < MAX_FLOAT_STEPS) {
		float oldX = x;

		x *= 10;

		if (x < oldX) mathState.bOverflow = true;
		
		result++;
	}

	return result;
}

uint16 getCountDecimalPlaces(double x) {
	uint16 result = 0;

	while (getFirstNumberAfterDecimalPoint(x) != 0 && result < MAX_DOUBLE_STEPS) {
		double oldX = x;

		x *= 10;

		if (x < oldX) mathState.bOverflow = true;
		
		result++;
	}

	return result;
}

double mod(double a, double b) {
	if (b == 0.0) {
		// throw(ERROR_DIVISION_BY_ZERO);

		return 0.0;
	}

	return MOD(a, b);
}

float nblk__fmod(float a, float b) {
	if (b == 0.0f) {
		// throw(ERROR_DIVISION_BY_ZERO);

		return 0.0f;
	}

	return FMOD(a, b);
}

_size_t align_down(_size_t x, _size_t align) {
	if (align == 0) {
		// throw(ERROR_DIVISION_BY_ZERO);

		return x;
	}

	if (x % align == 0) return x;

	return (x / align) * align;
}

_size_t align_up(_size_t x, _size_t align) {
	if (align == 0) {
		// throw(ERROR_DIVISION_BY_ZERO);

		return x;
	}

	if (x % align == 0) return x;

	return ((x / align) + 1) * align;
}

int32 pow32(int32 a, int32 b) {
	if (b < 0) return 0;

	if (b == 0) return 1;

	if (b == 1) return a;

	int32 result = 1;

	while (--b) {
		int32 oldResult = result;

		result *= a;

		if (result < oldResult) mathState.bOverflow = true;
	}

	return result;
}

_size_t powU32(_size_t a, _size_t b) {
	if (b == 0) return 1;

	if (b == 1) return a;

	_size_t result = 1;

	while (b--) {
		_size_t oldResult = result;

		result *= a;

		if (result < oldResult) mathState.bOverflow = true;
	}

	return result;
}

int64 pow64(int64 a, int64 b) {
	if (b < 0) return 0;

	if (b == 0) return 1;

	if (b == 1) return a;

	int64 result = 1;

	while (b--) {
		int64 oldResult = result;

		result *= a;

		if (result < oldResult) mathState.bOverflow = true;
	}

	return result;
}

uint64 powU64(uint64 a, uint64 b) {
	if (b == 0) return 1;

	if (b == 1) return a;

	uint64 result = 1;

	while (b--) {
		uint64 oldResult = result;

		result *= a;

		if (result < oldResult) mathState.bOverflow = true;
	}

	return result;
}