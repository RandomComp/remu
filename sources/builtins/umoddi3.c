#include "builtins/quad.h"

uint64 __umoddi3(uint64 a, uint64 b) {
	uint64 r;

	__qdivrem(a, b, &r);
	
	return r;
}