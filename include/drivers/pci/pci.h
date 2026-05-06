#ifndef _EMULATOR_OS_PCI_H
#define _EMULATOR_OS_PCI_H

#include "types.h"

#define PCI_CONFIG_ADDR 			0xCF8
#define PCI_CONFIG_DATA 			0xCFC

/*
Low word -- vendor id
High word -- device id
*/
#define PCI_CONFIG_VENDOR_ID 		0x00
/*
Low word -- subsystem vendor id
High word -- subsystem device id
*/
#define PCI_CONFIG_SUBSYS_ID 		0x2C

#define PCI_STORAGE 	0x01
#define PCI_NETWORK 	0x02
#define PCI_DISPLAY 	0x03
#define PCI_MULTIMEDIA 	0x04
#define PCI_BRIDGE 		0x06
#define PCI_SERIAL_BUS 	0x0C

#define PCI_STORAGE_IDE 			0x01
#define PCI_STORAGE_AHCI 			0x06

#define PCI_NETWORK_ETHERNET 		0x00
#define PCI_NETWORK_TOKEN_RING 		0x01
#define PCI_NETWORK_FDDI 			0x02
#define PCI_NETWORK_ATM				0x03
#define PCI_NETWORK_ISDN 			0x04
#define PCI_NETWORK_WORLDFIP		0x05
#define PCI_NETWORK_PICMG 			0x06
#define PCI_NETWORK_INFINIBAND 		0x07
#define PCI_NETWORK_FABRIC 			0x08
#define PCI_NETWORK_CONTROLLER 		0x80

#define PCI_DISPLAY_VGA 			0x00

#define PCI_MULTIMEDIA_VIDEO_CONTROLLER	0x00
#define PCI_MULTIMEDIA_AUDIO_CONTROLLER	0x01
#define PCI_MULTIMEDIA_TELEPHONY		0x02
#define PCI_MULTIMEDIA_AUDIO_DEVICE		0x03
#define PCI_MULTIMEDIA_CONTROLLER		0x80

#define PCI_BRIDGE_HOST 			0x00
#define PCI_BRIDGE_ISA	 			0x01
#define PCI_BRIDGE_EISA 			0x02
#define PCI_BRIDGE_MICRO_CHANNEL 	0x03
#define PCI_BRIDGE_PCI	 			0x04
#define PCI_BRIDGE_PCMCIA 			0x05
#define PCI_BRIDGE_NUBUS 			0x06
#define PCI_BRIDGE_CARDBUS 			0x07
#define PCI_BRIDGE_RACEWAY 			0x08
#define PCI_BRIDGE_SEMI_PCI_TO_PCI 	0x09
#define PCI_BRIDGE_INFINIBAND 		0x0A
#define PCI_BRIDGE_BRIDGE 			0x80

#define PCI_SERIAL_BUS_USB 			0x03

#define PCI_VENDOR_EMU				0xEA32
#define PCI_VENDOR_INTEL			0x8086
#define PCI_VENDOR_QEMU				0x1234
#define PCI_VENDOR_VMWARE			0x15AD
#define PCI_VENDOR_NVIDIA 			0x10DE
#define PCI_VENDOR_ATI				0x1002
#define PCI_VENDOR_AMD				0x1022
#define PCI_VENDOR_ENSONIQ 			0x1274
#define PCI_VENDOR_REALTEK 			0x10EC
#define PCI_VENDOR_QUALCOMM			0x168C

typedef struct pci_device_name_t {
	uint32 category;
	uint32 subclass;
	const byte* name;
} pci_device_name_t;

typedef struct pci_vendor_t {
	uint16 vendor;
	uint16 device;

	uint16 subsys_vendor;
	uint16 subsys_device;
	
	const byte* vendor_name;
	const byte* device_name;
	
	const byte* subsys_vendor_name;
	const byte* subsys_device_name;
} pci_vendor_t;

uint32 pci_read_config(byte bus, byte slot, byte func, byte offset);

void pci_show_device_info(byte bus, byte slot, byte func);

void pci_scan(void);

#endif
