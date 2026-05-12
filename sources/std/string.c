#include "std/string.h"

#include "types.h"

#include "builtins/string.h"
#include "math/math.h"

bool isupper(byte c) {
	return (c >= 'A') && (c <= 'Z');
}

bool islower(byte c) {
	return (c >= 'a') && (c <= 'z');
}

bool isnum(byte c) {
	return (c >= '0') && (c <= '9');
}

bool isalnum(byte c) {
	return isalpha(c) || isnum(c);
}

bool isalpha(byte c) {
	return islower(c) || isupper(c);
}

bool isascii(byte c) {
	return (c >= ' ') && (c <= 126);
}

bool isprintable(byte c) {
	return (c >= '!') && (c <= 254);
}

byte upper(byte c) {
	return islower(c) ? (c - 'a' + 'A') : c;
}

byte lower(byte c) {
	return isupper(c) ? (c - 'A' + 'a') : c;
}

static byte* strtok_str = nullptr; static size_t str_len = 0;

static ssize_t strtok_index = 0;

byte* strtok(byte* _str, const byte* delim) {
	if (!_str) {
		if (!strtok_str) return nullptr;

		if (strtok_index < 0) return nullptr;

		for (size_t i = strtok_index + 1; i < str_len; i++) {
			if (strtok_str[i] == 0) {
				byte* result = strtok_str + i + 1;

				strtok_index = i;

				return result;
			}
		}

		strtok_index = 0;
		
		return nullptr;
	}

	strtok_str = _str;
	
	size_t delim_len = strlen(delim);

	str_len = strlen(_str);

	for (size_t i = 0; _str[i]; i++) {
		size_t min_size = MIN(delim_len, str_len - 1);

		if (strncmp(_str + i, delim, min_size) == 0) {
			memset(_str + i, 0, min_size);

			i += min_size - 1;
		}
	}

	return _str;
}

byte* parse_cli_args(byte* _str) {
	if (!_str) {
		if (!strtok_str) return nullptr;

		if (strtok_index < 0) return nullptr;

		for (size_t i = strtok_index + 1; i < str_len; i++) {
			if (strtok_str[i] == 0) {
				for (; i < str_len; i++) {
					if (strtok_str[i] != 0) break;
				}

				byte* result = strtok_str + i;

				strtok_index = i;

				return result;
			}
		}

		strtok_index = 0;
		
		return nullptr;
	}

	str_len = strlen(_str);

	strtok_str = _str;

	bool quotes = false;
	bool double_quotes = false;

	bool escape = false;

	for (size_t i = 0; _str[i]; i++) {
		if (_str[i] == '"') {
			if (!escape) {
				double_quotes = !double_quotes;

				_str[i] = 0;
			}

			else escape = false;
		}

		if (_str[i] == '\'') {
			if (!escape) {
				quotes = !quotes;

				_str[i] = 0;
			}

			else escape = false;
		}

		if (_str[i] == '#' && !quotes && !double_quotes) {
			_str[i] = 0;

			str_len = i; break;
		}

		if (_str[i] == '\\') {
			escape = true;

			// _str[i] = 0;

			size_t k = i;

			for (size_t j = i; _str[j] != ' ' && j < str_len; j++) {
				if (_str[j] != '\\') {
					_str[k] = _str[j];

					k++;
				}
			}

			for (size_t j = k; _str[j] != ' ' && j < str_len; j++) {
				_str[j] = ' ';
			}
		}

		if ((_str[i] == ' ' || _str[i] == '\t') && !quotes && !double_quotes) {
			_str[i] = 0;
		}
	}

	return _str;
}

void strip_str(byte* str, size_t size) {
	for (size_t i = size; str[i] == ' '; i--) {
		if (str[i] != ' ') break;

		str[i] = 0;
	}
}
