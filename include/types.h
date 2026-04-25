#ifndef _EMULATOR_OS_BASIC_TYPES_H
#define _EMULATOR_OS_BASIC_TYPES_H

// Полностью заимствованно из Random OS Boosted

#define true 1

#define false 0

#define PACKED __attribute__((packed))

#define UNUSED __attribute__((unused))

#define INT8_MIN (int8)(0x80)

#define INT16_MIN (int16)(0x8000)

#define INT32_MIN (int32)(0x80000000)

#define INT64_MIN (int64)(0x8000000000000000)

#define UINT8_MIN (uint8)(0)

#define UINT16_MIN (uint16)(0)

#define UINT32_MIN (uint32)(0)

#define UINT64_MIN (uint64)(0)

#define INT8_MAX (int8)(0x7F)

#define INT16_MAX (int16)(0x7FFF)

#define INT32_MAX (int32)(0x7FFFFFFF)

#define INT64_MAX (int64)(0x7FFFFFFFFFFFFFFF)

#define UINT8_MAX (uint8)(0xFF)

#define UINT16_MAX (uint16)(0xFFFF)

#define UINT32_MAX (uint32)(0xFFFFFFFF)

#define UINT64_MAX (uint64)(0xFFFFFFFFFFFFFFFF)

#define FLOAT_MIN (double)(1.175494351e-38)

#define DOUBLE_MIN (double)(2.2250738585072014e-308)

#define FLOAT_MAX (double)(3.402823466e+38)

#define DOUBLE_MAX (double)(1.7976931348623158e+308)

#define FLT_EPSILON (float)(1.192092896e-07f)

#define DBL_EPSILON (double)(2.2204460492503131e-16)

#define FLT_MIN (FLOAT_MIN)

#define DBL_MIN (DOUBLE_MIN)

#define FLT_MAX (FLOAT_MAX)

#define DBL_MAX (DOUBLE_MAX)

#define MAX_FLOAT_STEPS 7

#define MAX_DOUBLE_STEPS 17

typedef enum ErrorCode {
	CODE_OK = 0,
	CODE_FAIL = 1
} ErrorCode;

#define null 0

#define nullptr (void*)null

#define DEBUG

#define OS_LITTLE_ENDIAN

// #define OS_BIG_ENDIAN

#if !defined(__STDC_HOSTED__) || __STDC_HOSTED__ == 0
	#define FREE_STANDING_MODE
#endif

// #define BITS_16

#ifdef FREE_STANDING_MODE
#define BITS_32
#else
#define BITS_64
#endif

// BITS 16 is unsupported now.

typedef unsigned char uint8;
typedef unsigned short uint16;

typedef signed char int8;
typedef signed short int16;

// Unsupported for 16 BITS, use uint with dynamical depth for this.
#if defined(BITS_32) || defined(BITS_64)
	typedef unsigned long long uint64;
	
	typedef unsigned int uint32;
	typedef unsigned long long uint64;

	typedef signed long int32;
	typedef signed long long int64;
	
	// typedef int32 int32_t;
	// typedef int64 int64_t;

	typedef uint32 dword;
	typedef uint64 qword;
#endif

#if defined(BITS_16)

typedef uint16 _size_t;
typedef int16 _ssize_t;

#elif defined(BITS_32)

typedef uint32 _size_t;
typedef int32 _ssize_t;

#elif defined(BITS_64)

typedef uint64 size_t;
typedef int64 ssize_t;

#endif

typedef size_t word;

typedef char* c_str;

typedef c_str* c_str_ptr;

typedef _Bool bool;

typedef uint8 byte;

typedef int8 intmin_t;
typedef int64 intmax_t;

typedef uint8 uintmin_t;
typedef uint64 uintmax_t;

typedef ssize_t time_t;

#define static_assert _Static_assert

#undef va_start
#undef va_end
#undef va_arg
#undef va_copy
#undef va_list

#define va_start(PTR, LASTARG)  __builtin_va_start(PTR, LASTARG)
#define va_end(PTR)             __builtin_va_end(PTR)
#define va_arg(PTR, TYPE)       __builtin_va_arg(PTR, TYPE)
#define va_copy(dest, src) 		__builtin_va_copy(dest, src)
#define va_list                 __builtin_va_list

#endif
