#include "types.h"

#include "screen/emulator_vesa_gpu.h"

#include "memory/emulator_ram.h"

#include <malloc.h>

#include <string.h>

static vesa_device_t* cur = nullptr;

vesa_device_t* init_vesa_device(_size_t width, _size_t height, _size_t bpp) {
	vesa_device_t* vesa_device = malloc(sizeof(vesa_device_t));

	memset(vesa_device, 0, sizeof(vesa_device_t));

	vesa_device->width = width;
	vesa_device->height = height;
	vesa_device->bpp = bpp;

	size_t vidmem_size = vesa_device->width * vesa_device->height * vesa_device->bpp / 8;

	vesa_device->vidmem = malloc(vidmem_size);

	memset(vesa_device->vidmem, 0, vidmem_size);

	return vesa_device;
}

void free_vesa_device(vesa_device_t* vesa_device) {
	if (!vesa_device) return;
	
	if (vesa_device->vidmem) {
		free(vesa_device->vidmem); vesa_device->vidmem = nullptr;
	}

	free(vesa_device);

	if (cur == vesa_device)
		cur = nullptr;
}
