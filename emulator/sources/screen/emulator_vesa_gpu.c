#include "types.h"

#include "screen/emulator_vesa_gpu.h"

#include "pci/emulator_pci.h"

#include "emulator_logger.h"

#include <malloc.h>

#include <string.h>

static vesa_device_t* cur = nullptr;

vesa_device_t* init_vesa_device(_size_t width, _size_t height, _size_t bpp) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VESA GPU initializing...");

	vesa_device_t* vesa_device = malloc(sizeof(vesa_device_t));

	memset(vesa_device, 0, sizeof(vesa_device_t));

	vesa_device->width = width;
	vesa_device->height = height;
	vesa_device->bpp = bpp;

	size_t vidmem_size = vesa_device->width * vesa_device->height * vesa_device->bpp / 8;

	vesa_device->vidmem = malloc(vidmem_size);

	memset(vesa_device->vidmem, 0, vidmem_size);

	vesa_device->pci_device = pci_init_device(
		0, 0, 0,
		PCI_DISPLAY, PCI_DISPLAY_VGA,
		PCI_VENDOR_EMU,
		0x4444,
		PCI_VENDOR_EMU,
		0x4444
	);

	pci_device_register(nullptr, vesa_device->pci_device);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "VESA GPU registered in PCI");

	emulator_log(true, LOG_SEVERITY_VERBOSE, "VESA GPU initialized with video mode %ux%ux%u", width, height, bpp);

	return vesa_device;
}

void free_vesa_device(vesa_device_t* vesa_device) {
	if (!vesa_device) return;
	
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VESA GPU deinitializing...");

	if (vesa_device->vidmem) {
		free(vesa_device->vidmem); vesa_device->vidmem = nullptr;
	}

	if (vesa_device->pci_device) {
		free_pci_device(vesa_device->pci_device); vesa_device->pci_device = nullptr;
	}

	free(vesa_device);

	if (cur == vesa_device)
		cur = nullptr;
	
	emulator_log(true, LOG_SEVERITY_VERBOSE, "VESA GPU deinitialized");
}
