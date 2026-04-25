#ifndef _EMULATOR_OS_BCD_H
#define _EMULATOR_OS_BCD_H

#include "types.h"

static _size_t to_bcd(_size_t num) {
	_size_t result = 0;

	_size_t index = 0;

	while (num >= 1) {
		result += (num % 10) * (1 << index);
		
		index += 4;

		num /= 10;
	}

	return result;
}

static _size_t from_bcd(_size_t num) {
	_size_t result = 0;

	_size_t index = 0;

	while (num >= 1) {
		_size_t temp_result = num % 16;

		for (_size_t i = 0; i < index; i++) {
			temp_result *= 10;
		}

		result += temp_result;

		index += 1;

		num /= 16;
	}

	return result;
}

#endif
