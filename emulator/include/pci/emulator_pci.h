#ifndef _EMULATOR_PCI_H
#define _EMULATOR_PCI_H

#include <stddef.h>

#include "pci/emulator_pci_fwd.h"

struct pci_device_t {
	uint32 reg[252];

	byte bus;
	byte slot;
	byte function;

	bool active;
};

struct pci_t {
	pci_device_t* devices[8]; size_t device_index;
};

pci_t* init_pci(void);

pci_device_t* pci_find_device(pci_t* pci, byte bus, byte slot, byte function);

pci_device_t* pci_init_device(	byte bus, byte slot, 	byte function,
								byte category, 			byte subclass, 
								uint16 vendor, 			uint16 device, 
								uint16 subsys_vendor, 	uint16 subsys_device);

byte pci_device_register(pci_t* _pci, pci_device_t* device);

void pci_device_deregister(pci_t* _pci, byte bus, byte slot, byte function);

void free_pci_device(pci_device_t* device);

void free_pci(pci_t* pci);

#endif
