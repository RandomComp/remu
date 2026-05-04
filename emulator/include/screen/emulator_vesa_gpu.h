#ifndef _EMULATOR_VESA_GPU_H
#define _EMULATOR_VESA_GPU_H

#include "types.h"

typedef struct ram_t ram_t;

typedef struct vesa_device_t {
	_size_t width;
	_size_t height;
	_size_t bpp;

	byte* vidmem;
} vesa_device_t;

vesa_device_t* init_vesa_device(_size_t width, _size_t height, _size_t bpp);

void free_vesa_device(vesa_device_t* screen);

#endif
