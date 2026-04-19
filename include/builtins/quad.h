#include "types.h"

union uu {
	int64 q;

	uint64 uq;

	int32 sl[2];

	uint32 ul[2];
};

#define _QUAD_HIGHWORD 1

#define _QUAD_LOWWORD 0

#define	H _QUAD_HIGHWORD

#define	L _QUAD_LOWWORD

#define CHAR_BIT 8

#define	QUAD_BITS sizeof(int64) * CHAR_BIT

#define	INT_BITS sizeof(int32) * CHAR_BIT

#define	HALF_BITS sizeof(int32) * CHAR_BIT / 2

#define	HHALF(x) (((uint32)x) >> HALF_BITS)

#define	LHALF(x) (((uint32)x) & (((int32)1 << HALF_BITS) - 1))

#define	LHUP(x) (((uint32)x) << HALF_BITS)

int64 __adddi3(int64, int64);

int64 __anddi3(int64, int64);

int64 __ashldi3(int64, uint32);

int64 __ashrdi3(int64, uint32);

int32 __cmpdi2(int64, int64);

int64 __divdi3(int64, int64);

int64 __fixdfdi(double);

int64 __fixsfdi(float);

uint64 __fixunsdfdi(double);

uint64 __fixunssfdi(float);

double __floatdidf(int64);

float __floatdisf(int64);

double __floatunsdidf(uint64);

int64 __iordi3(int64, int64);

int64 __lshldi3(int64, uint32);

int64 __lshrdi3(int64, uint32);

int64 __moddi3(int64, int64);

int64 __muldi3(int64, int64);

int64 __negdi2(int64);

int64 __one_cmpldi2(int64);

uint64 __qdivrem(uint64, uint64, uint64*);

int64 __subdi3(int64, int64);

int32 __ucmpdi2(uint64, uint64);

uint64 __udivdi3(uint64, uint64);

uint64 __umoddi3(uint64, uint64);

int64 __xordi3(int64, int64);