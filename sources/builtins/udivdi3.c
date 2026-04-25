#include "builtins/quad.h"

uint64 __udivdi3(uint64 a, uint64 b) {
	return __qdivrem(a, b, (uint64*)0);
}
