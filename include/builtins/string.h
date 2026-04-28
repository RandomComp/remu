#ifndef _EMULATOR_OS_BUILTINS_STRING_H
#define _EMULATOR_OS_BUILTINS_STRING_H

#include "types.h"

#define USE_BUILTIN_STRING

static inline void* memset_slow(void* addr, int32 val, int32 len) {
	byte* p = (byte*)addr;

	while (len--)
		*p++ = val;

	return addr;
}

static inline void* memcpy_slow(void* dest, void* src, int32 len) {
	byte* p;

	byte* q;

	p = dest;

	q = src;

	while (len--)
		*p++ = *q++;

	return dest;
}

static inline int32 strcmp_slow(byte* s1, byte* s2) {
	int32 r, c1, c2;

	do {
		c1 = *s1++;

		c2 = *s2++;

		r = c1 - c2;
	} while (!r && c1);

	return r;
}

static inline int32 memcmp_slow(void* p1, void* p2, int32 len) {
	int32 r, i;

	byte* q1;

	byte* q2;

	q1 = p1;

	q2 = p2;

	for (r = 0, i = 0; !r && i < len; i++)
		r = *q1++ - *q2++;

	return r;
}

static inline int32 strlen_slow(byte* p) {
	int32 len = 0;

	while (*p++)
		len++;

	return len;
}

static inline int32 strncmp_slow(byte* s1, byte* s2, int32 len) {
	int32 r, c1, c2;

	if (len <= 0)
		return 0;

	do {
		c1 = *s1++;

		c2 = *s2++;

		r = c1 - c2;
	} while (!r && c1 && --len > 0);

	return r;
}

#ifdef USE_BUILTIN_STRING
#       define memset(addr, val, len)   memset_builtin(addr, val, len)
#       define memcpy(dest, src, len)   memcpy_builtin(dest, src, len)
#       define strcmp(s1, s2)           strcmp_builtin(s1, s2)
#       define memcmp(p1, p2, len)      memcmp_builtin(p1, p2, len)
#       define strlen(p)                strlen_builtin(p)
#       define strncmp(s1, s2, len)     strncmp_builtin(s1, s2, len)
#else
#       define memset(addr, val, len)   memset_slow(addr, val, len)
#       define memcpy(dest, src, len)   memcpy_slow(dest, src, len)
#       define strcmp(s1, s2)           strcmp_slow(s1, s2)
#       define memcmp(p1, p2, len)      memcmp_slow(p1, p2, len)
#       define strlen(p)                strlen_slow(p)
#       define strncmp(s1, s2, len)     strncmp_slow(s1, s2, len)
#endif

#ifdef USE_BUILTIN_STRING

static inline void* memset_builtin(void* addr, int32 val, int32 len) {
	return __builtin_memset(addr, val, len);
}

static inline void* memcpy_builtin(void* dest, void* src, int32 len) {
	return __builtin_memcpy(dest, src, len);
}

static inline int32 strcmp_builtin(byte* s1, byte* s2) {
	return __builtin_strcmp(s1, s2);
}

static inline int32 memcmp_builtin(void* p1, void* p2, int32 len) {
	if (__builtin_constant_p(len) && len == 2) {
		uint16* q1 = p1, *q2 = p2;

		return *q1 == *q2 ? 0 : __builtin_memcmp(p1, p2, len) | 1;
	}

	if (__builtin_constant_p(len) && len == 4) {
		uint32* q1 = p1,* q2 = p2;

		return *q1 == *q2 ? 0 : __builtin_memcmp(p1, p2, len) | 1;
	}

	return __builtin_memcmp(p1, p2, len);
}

static inline int32 strlen_builtin(byte* p) {
	return __builtin_strlen(p);
}

static inline int32 strncmp_builtin(byte* s1, byte* s2, int32 len) {
	return __builtin_strncmp(s1, s2, len);
}

#endif

#endif
