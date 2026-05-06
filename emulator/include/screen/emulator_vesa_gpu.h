#ifndef _EMULATOR_VESA_GPU_H
#define _EMULATOR_VESA_GPU_H

#include "types.h"

#include "pci/emulator_pci_fwd.h"

typedef struct vesa_device_t {
	_size_t width;
	_size_t height;
	_size_t bpp;

	byte* vidmem;

	pci_device_t* pci_device;
} vesa_device_t;

vesa_device_t* init_vesa_device(_size_t width, _size_t height, _size_t bpp);

void free_vesa_device(vesa_device_t* screen);

#endif
