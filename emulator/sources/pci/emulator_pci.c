#include "types.h"

#include "emulator_io.h"

#include "emulator_logger.h"

#include "pci/emulator_pci.h"

#include <malloc.h>

#include <string.h>

static _size_t addr = 0;

static pci_t* cur = nullptr;

static void pci_write_address(_size_t data) {
	if (data & (1 << 31)) {
		addr = data & 0x7FFFFFFF; // (~(1 << 31)) & 0xFFFFFFFF == 0x7FFFFFFF

		byte bus = (addr >> 16) & 0xFF;
		byte slot = (addr >> 11) & 0x1F;
		byte function = (addr >> 8) & 0x07;
		byte offset = addr & 0xFC;

		emulator_log(false, LOG_SEVERITY_TRACE, "Writed address %.4x (bus %.2x, slot %.2x, function %.2b) to PCI interface.", addr, bus, slot, function);
	}
}


static _size_t pci_read_data(void) {
	byte bus = 		(addr >> 16) 	& 0xFF;
	byte slot = 	(addr >> 11) 	& 0x1F;
	byte function = (addr >> 8) 	& 0x07;
	byte offset = 	(addr) 			& 0xFC;

	emulator_log(false, LOG_SEVERITY_TRACE, "Searching %.4x (bus %.2x, slot %.2x, function %.2b) in PCI interface...", addr, bus, slot, function);

	pci_device_t* device = pci_find_device(cur, bus, slot, function);

	if (!device) {
		emulator_log(false, LOG_SEVERITY_TRACE, "Device %.4x not founded in PCI interface", addr);
	
		return 0xFFFF;
	}

	emulator_log(false, LOG_SEVERITY_TRACE, "Device %.8x founded in PCI interface", addr);
	
	_size_t result = device->reg[offset];

	return result;
}

pci_t* init_pci(void) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "PCI initializing...");

	pci_t* pci = malloc(sizeof(pci_t));

	memset(pci, 0, sizeof(pci_t));

	pci->device_index = 0;

	cur = pci;

	emulator_setup_port_out(PCI_CONFIG_ADDR, pci_write_address);
	
	emulator_setup_port_in(PCI_CONFIG_DATA, pci_read_data);

	emulator_log(true, LOG_SEVERITY_VERBOSE, "PCI initialized!");

	return pci;
}

pci_device_t* pci_find_device(pci_t* _pci, byte bus, byte slot, byte function) {
	pci_t* pci = _pci;

	if (!pci) pci = cur;

	if (!pci) {
		emulator_log(true, LOG_SEVERITY_WARNING, "Tried to find PCI device using uninitiailized PCI interface.");

		return nullptr;
	}

	size_t pci_devices_max_cnt = sizeof(pci->devices) / sizeof(pci->devices[0]);

	for (size_t i = 0; i < pci_devices_max_cnt; i++) {
		pci_device_t* device = pci->devices[i];

		if (!device) continue;

		if (device->active &&
			device->bus == bus &&
			device->slot == slot &&
			device->function == function) {
			return device;
		}
	}

	return nullptr;
}

pci_device_t* pci_init_device(	byte bus, byte slot, 	byte function,
								byte category, 			byte subclass, 
								uint16 vendor, 			uint16 device, 
								uint16 subsys_vendor, 	uint16 subsys_device) {
	pci_device_t* pci_device = malloc(sizeof(pci_device_t));

	memset(pci_device, 0, sizeof(pci_device_t));
	
	pci_device->active = true;
	
	pci_device->bus = bus;
	pci_device->slot = slot;
	pci_device->function = function;

	pci_device->reg[PCI_CONFIG_CATEGORY] = ((uint32)category << 24) | 
											((uint32)subclass << 16);

	pci_device->reg[PCI_CONFIG_VENDOR_ID] = (uint32)vendor | 
											((uint32)device << 16);

	pci_device->reg[PCI_CONFIG_SUBSYS_ID] = (uint32)subsys_vendor | 
											((uint32)subsys_device << 16);

	return pci_device;
}



byte pci_device_register(pci_t* _pci, pci_device_t* device) {
	pci_t* pci = _pci;

	if (!pci) pci = cur;

	if (!pci) {
		emulator_log(true, LOG_SEVERITY_WARNING, "Tried to register PCI device using uninitiailized PCI interface.");

		return 0;
	}

	size_t pci_devices_max_cnt = sizeof(pci->devices) / sizeof(pci->devices[0]);

	if (pci->device_index >= pci_devices_max_cnt) {
		emulator_log(true, LOG_SEVERITY_WARNING, "Tried to register PCI device, but all slots are occupied.");

		return 0;
	}

	byte index = pci->device_index;

	pci->devices[index] = device;

	pci->devices[index]->slot = index % 32;
	pci->devices[index]->bus = index / 32;

	pci->device_index++;

	return index;
}

void pci_device_deregister(pci_t* _pci, byte bus, byte slot, byte function) {
	pci_t* pci = _pci;

	if (!pci) pci = cur;

	if (!pci) {
		emulator_log(true, LOG_SEVERITY_WARNING, "Tried to deregister PCI device using uninitiailized PCI interface.");

		return;
	}

	pci_device_t* device = pci_find_device(pci, bus, slot, function);

	if (!device) {
		emulator_log(true, LOG_SEVERITY_WARNING, "Tried to deregister unregistered PCI device on %.2x:%.2x:%.2x", bus, slot, function);

		return;
	}

	memset(device, 0xFF, sizeof(pci_device_t));
}

void free_pci_device(pci_device_t* device) {
	if (!device) return;

	if (cur) {
		for (size_t i = 0; i < sizeof(cur->devices) / sizeof(cur->devices[0]); i++) {
			if (cur->devices[i] == device) {
				cur->devices[i] = nullptr;
			}
		}
	}

	free(device);
}

void free_pci(pci_t* _pci) {
	pci_t* pci = _pci;

	if (!pci) pci = cur;

	emulator_log(true, LOG_SEVERITY_VERBOSE, "PCI deinitializing...");

	emulator_release_port_out(PCI_CONFIG_ADDR);

	emulator_release_port_in(PCI_CONFIG_DATA);

	if (!pci) return;

	free(pci);

	if (cur == pci)
		cur = nullptr;

	emulator_log(true, LOG_SEVERITY_VERBOSE, "PCI deinitialized");
}
