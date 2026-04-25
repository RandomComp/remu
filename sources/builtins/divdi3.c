#include "types.h"

#include "builtins/quad.h"

int64 __divdi3(int64 a, int64 b) {
	uint64 ua, ub, uq;

	bool al = a < 0, bl = b < 0;

	ua = al ? -a : a;

	ub = bl ? -b : b;

	uq = __qdivrem(ua, ub, (uint64*)0);
	
	return al || bl ? -uq : uq;
}
