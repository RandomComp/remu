#ifndef _EMULATOR_BASIC_TYPES_H
#define _EMULATOR_BASIC_TYPES_H

// ╨ñ╨░╨║╤é╨╕╤ç╨╡╤ü╨║╨╕ ╨╖╨░╨╕╨╝╤ü╤é╨▓╨╛╨▓╨░╨╜╨╜╨╛ ╨╕╨╖ R-OS Boosted (╤Ç╨░╨╜╨╡╨╡ Random OS Boosted)

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

#if (defined(_WIN32) || defined(_WIN64)) || (defined(__MINGW32__) || defined(__MINGW64__))
#define IS_WIN 
#elif (defined(__linux__) || defined(__APPLE__) || defined(__unix__) || defined(__unix) || defined(__FreeBSD__) || defined(__ANDROID__))
#define IS_UNIX
#endif

#if (defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__))
#define PLATFORM_NAME "Windows"
#elif (defined(__ANDROID__))
#define PLATFORM_NAME "Android"
#elif (defined(__APPLE__) || defined(__MACH__))
#define PLATFORM_NAME "MacOS"
#elif (defined(__linux__))
#define PLATFORM_NAME "Linux"
#elif (defined(__unix__) || defined(__unix))
#define PLATFORM_NAME "UNIX compatible"
#elif (defined(_POSIX_VERSION))
#define PLATFORM_NAME "POSIX compatible"
#endif

#if (defined(i386) || defined(__i386__) || defined(_X86_))
#define PLATFORM_ARCH "x86-32"
#define PLATFORM_X86_32
#elif (defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64))
#define PLATFORM_ARCH "x86-64"
#define PLATFORM_X86_64
#elif (defined(__arm__) || defined(__ARMEL__))
#define PLATFORM_ARCH "ARM-32"
#define PLATFORM_ARM_32
#elif (defined(__aarch64__))
#define PLATFORM_ARCH "AARCH-64"
#define PLATFORM_AARCH_64
#elif (defined(__riscv) || defined(__riscv__))
#define PLATFORM_ARCH "RISC-V"
#define PLATFORM_RISC_V
#endif

#if (defined(__clang__))
#define PLATFORM_COMPILER_NAME "Clang"
#define PLATFORM_COMPILER_VERSION_MINOR __clang_minor__
#define PLATFORM_COMPILER_VERSION_MAJOR __clang_major__
#elif (defined(__INTEL_COMPILER))
#define PLATFORM_COMPILER_NAME "Intel compiler"
#define PLATFORM_COMPILER_VERSION_MINOR (__INTEL_COMPILER % 100)
#define PLATFORM_COMPILER_VERSION_MAJOR (__INTEL_COMPILER / 100)
#elif (defined(__MINGW32__) || defined(__MINGW64__))
#define PLATFORM_COMPILER_NAME "MINGW"
#define PLATFORM_COMPILER_VERSION_MINOR __GNUC_MINOR__
#define PLATFORM_COMPILER_VERSION_MAJOR __GNUC__
#elif (defined(__GNUC__) || defined(__GNUC_MINOR__) || defined(__GNUC_PATCHLEVEL__))
#define PLATFORM_COMPILER_NAME "GCC"
#define PLATFORM_COMPILER_VERSION_MINOR __GNUC_MINOR__
#define PLATFORM_COMPILER_VERSION_MAJOR __GNUC__
#elif (defined(_MSC_VER))
#define PLATFORM_COMPILER_NAME "MSVC"
#define PLATFORM_COMPILER_VERSION_MINOR (_MSC_VER / 100)
#define PLATFORM_COMPILER_VERSION_MAJOR (_MSC_VER % 100)
#endif

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
#endif

#include <sys/types.h>

// typedef size_t _size_t;

#if defined(BITS_16)

typedef uint16 _size_t;
typedef int16 _ssize_t;

#elif defined(BITS_32)

typedef uint32 _size_t;
typedef int32 _ssize_t;

#elif defined(BITS_64)

typedef uint64 _size_t;
typedef int64 _ssize_t;

#endif

typedef _size_t word;

typedef _Bool bool;

typedef uint8 byte;

typedef _ssize_t _time_t;

#endif
