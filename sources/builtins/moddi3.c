#include "types.h"

#include "builtins/quad.h"

int64 __moddi3(int64 a, int64 b) {
	uint64 ua, ub, ur;

	bool al = a < 0, bl = b < 0;

	ua = al ? -a : a;

	ub = bl ? -b : b;

	__qdivrem(ua, ub, &ur);

	return al || bl ? -ur : ur;
}