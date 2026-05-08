#include "types.h"

#include "drivers/pci/pci.h"

#include "drivers/io.h"

#include "math/math.h"

#include "std.h"

uint32 pci_read_config(byte bus, byte slot, byte func, byte offset) {
	uint32 address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);

	out32(PCI_CONFIG_ADDR, address);

	return in32(PCI_CONFIG_DATA);
}

static pci_device_name_t device_names[] = {
	{PCI_STORAGE, 		PCI_STORAGE_SCSI, 				"SCSI Controller"},
	{PCI_STORAGE, 		PCI_STORAGE_IDE, 				"IDE Controller"},
	{PCI_STORAGE, 		PCI_STORAGE_FLOPPY, 			"Floppy Controller"},
	{PCI_STORAGE, 		PCI_STORAGE_IPI, 				"IPI Bus Controller"},
	{PCI_STORAGE, 		PCI_STORAGE_RAID_BUS, 			"RAID Bus Controller"},
	{PCI_STORAGE, 		PCI_STORAGE_ATA, 				"ATA Controller"},
	{PCI_STORAGE, 		PCI_STORAGE_SATA, 				"SATA Controller"},
	{PCI_STORAGE, 		PCI_STORAGE_SRL_ATCH_SCSI, 		"Serial Attached SCSI Controller"},
	{PCI_STORAGE, 		PCI_STORAGE_NON_VLTL_MMR, 		"Non-Volatile memory controller"},
	{PCI_STORAGE, 		PCI_STORAGE_FLASH_MEMORY, 		"Flash Storage controller"},
	{PCI_STORAGE, 		PCI_STORAGE_CONTROLLER, 		"Storage controller"},

	{PCI_NETWORK, 		PCI_NETWORK_ETHERNET, 			"Ethernet controller"},
	{PCI_NETWORK, 		PCI_NETWORK_TOKEN_RING, 		"Token ring controller"},
	{PCI_NETWORK, 		PCI_NETWORK_FDDI, 				"FDDI controller"},
	{PCI_NETWORK, 		PCI_NETWORK_ATM, 				"ATM controller"},
	{PCI_NETWORK, 		PCI_NETWORK_ISDN, 				"ISDN controller"},
	{PCI_NETWORK, 		PCI_NETWORK_WORLDFIP, 			"WorldFip controller"},
	{PCI_NETWORK, 		PCI_NETWORK_PICMG, 				"PICMG controller"},
	{PCI_NETWORK, 		PCI_NETWORK_INFINIBAND, 		"InfiniBand controller"},
	{PCI_NETWORK, 		PCI_NETWORK_FABRIC, 			"Fabric controller"},
	{PCI_NETWORK, 		PCI_NETWORK_CONTROLLER, 		"Network controller"},

	{PCI_DISPLAY, 		PCI_DISPLAY_VGA, 				"VGA Compatible GPU"},
	{PCI_DISPLAY, 		PCI_DISPLAY_XGA, 				"XGA Compatible GPU"},
	{PCI_DISPLAY, 		PCI_DISPLAY_3D, 				"3D Accelerator (GPU)"},
	{PCI_DISPLAY, 		PCI_DISPLAY_CONTROLLER, 		"Display Controller"},

	{PCI_MULTIMEDIA, 	PCI_MULTIMEDIA_VIDEO_CONTROLLER,"Video controller"},
	{PCI_MULTIMEDIA, 	PCI_MULTIMEDIA_AUDIO_CONTROLLER,"Audio controller"},
	{PCI_MULTIMEDIA, 	PCI_MULTIMEDIA_TELEPHONY,		"Telephony device"},
	{PCI_MULTIMEDIA, 	PCI_MULTIMEDIA_AUDIO_DEVICE,	"Audio device"},
	{PCI_MULTIMEDIA, 	PCI_MULTIMEDIA_CONTROLLER,		"Multimedia controller"},

	{PCI_BRIDGE, 		PCI_BRIDGE_HOST,				"Host bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_ISA,					"ISA bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_EISA,				"EISA bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_MICRO_CHANNEL,		"Micro channel bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_PCI,					"PCI bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_PCMCIA,				"PCMCIA bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_NUBUS,				"NuBus bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_CARDBUS,				"CardBus bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_RACEWAY,				"RACEway bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_SEMI_PCI_TO_PCI,		"Semi-transparent PCI-to-PCI bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_INFINIBAND,			"InfiniBand to PCI host bridge"},
	{PCI_BRIDGE, 		PCI_BRIDGE_BRIDGE,				"Bridge"},

	{PCI_SERIAL, 		PCI_SERIAL_FIREWIRE, 			"FireWire (IEEE 1394)"},
	{PCI_SERIAL, 		PCI_SERIAL_ACCESS_BUS, 			"ACCESS Bus"},
	{PCI_SERIAL, 		PCI_SERIAL_SSA, 				"SSA"},
	{PCI_SERIAL, 		PCI_SERIAL_USB, 				"USB Serial"},
	{PCI_SERIAL, 		PCI_SERIAL_FIBRE, 				"Fibre controller"},
	{PCI_SERIAL, 		PCI_SERIAL_SMBUS, 				"SMBus"},
	{PCI_SERIAL, 		PCI_SERIAL_INFINIBAND, 			"InfiniBand"},
	{PCI_SERIAL, 		PCI_SERIAL_IPMI, 				"IPMI Interface"},
	{PCI_SERIAL, 		PCI_SERIAL_SERCOS, 				"SERCOS Interface"},
	{PCI_SERIAL, 		PCI_SERIAL_CANBUS, 				"CANBUS"},
	{PCI_SERIAL, 		PCI_SERIAL_MIPI_I3C, 			"MIPI I3C"},
	{PCI_SERIAL, 		PCI_SERIAL_CONTROLLER, 			"Serial Controller"},

	{PCI_ENCR_CONTR, 	PCI_ENCR_NET_COMPUTE,			"Network and computing encryption device"},
	{PCI_ENCR_CONTR, 	PCI_ENCR_ENTERTAINMENT,			"Entertainment encryption device"},
	{PCI_ENCR_CONTR, 	PCI_ENCR_CONTROLLER,			"Encryption controller"},

	{PCI_NON_ESSNTL, 	0x00,							"Non-essential instrumentation"},

	{0},
};

static pci_vendor_t vendor_names[] = {
	{
		PCI_VENDOR_INTEL, 	
		0x7010,
		0x1AF4,
		0x1100,
		"Intel", 
		"PIIX3 IDE [Natoma/Triton II]",
		"QEMU", "Hard drive",
	},
	{
		PCI_VENDOR_INTEL,
		0x100E,
		0x1AF4,
		0x1100,
		"Intel",
		"Gigabit Ethernet Controller",
		"QEMU", "Ethernet Controller"
	},
	{
		PCI_VENDOR_QEMU, 	
		0x1111, 
		0x1AF4,
		0x1100,
		"QEMU",
		"Video Adapter",
		"QEMU", "Video Adapter"
	},
	{
		PCI_VENDOR_VMWARE,
		0x0405,
		PCI_VENDOR_VMWARE,
		0x0405,
		"VMware",
		"SVGA II Adapter",
		"VMware",
		"SVGA II Adapter",
	},
	{
		PCI_VENDOR_VMWARE,
		0x07A0,
		0,
		0,
		"VMware",
		"PCI Express Root Port",
		0, 0,
	},
	{
		PCI_VENDOR_NVIDIA,
		0x1004,
		0,
		0,
		"NVIDIA",
		"GTX 780",
		0, 0,
	},
	{
		PCI_VENDOR_NVIDIA,
		0x119A,
		0,
		0,
		"NVIDIA",
		"GTX 860M",
		0, 0,
	},
	{
		PCI_VENDOR_NVIDIA,
		0x1392,
		0,
		0,
		"NVIDIA",
		"GTX 860M",
		0, 0,
	},
	{
		PCI_VENDOR_NVIDIA,
		0x139B,
		0,
		0,
		"NVIDIA",
		"GTX 960M",
		0, 0,
	},
	{
		PCI_VENDOR_NVIDIA,
		0x1F09,
		0,
		0,
		"NVIDIA",
		"GTX 1660 Super",
		0, 0,
	},
	{
		PCI_VENDOR_NVIDIA,
		0x1F0A,
		0x1043,
		0x879C,
		"NVIDIA",
		"GTX 1650",
		"ASUS",
		"TUF GTX 1650 Gaming",
	},
	{
		PCI_VENDOR_NVIDIA,
		0x10F9,
		0x1043,
		0x879C,
		"NVIDIA",
		"TU106 High Definition Audio Controller",
		"ASUS",
		"TUF GTX 1650 Gaming High Definition Audio Controller",
	},
	{
		PCI_VENDOR_ENSONIQ,
		0x1371,
		PCI_VENDOR_ENSONIQ,
		0x1371,
		"Ensoniq",
		"ES1371/ES1373 / Creative Labs CT2518",
		"Ensoniq",
		"Audio PCI 64V/128/5200 / Creative CT4810/CT5803/CT5806 [Sound Blaster PCI]",
	},
	{
		PCI_VENDOR_AMD,
		0x2000,
		0x1022,
		0x2000,
		"AMD",
		"PCnet32 LANCE",
		"AMD",
		"PCnet - Fast 79C971",
	},
	{
		PCI_VENDOR_ATI,
		0x6600,
		0, 0,
		"AMD",
		"Radeon HD 8670A/8670M/8750M",
		0, 0,
	},
	{
		PCI_VENDOR_REALTEK,
		0x8136,
		0, 0,
		"Realtek",
		"RTL810xE PCI Express Fast Ethernet controller",
		0, 0,
	},
	{
		PCI_VENDOR_QUALCOMM,
		0x002B,
		0, 0,
		"Qualcomm",
		"AR9285 Wireless Network Adapter (PCI-Express)",
		0, 0,
	},
	{
		PCI_VENDOR_INTEL,
		0x27BC,
		0, 0,
		"Intel",
		"NM10 Family LPC Controller",
		0, 0,
	},
	{
		PCI_VENDOR_INTEL,
		0x1237,
		0x1AF4,
		0x1100,
		"Intel",
		"440FX - 82441FX PMC [Natoma]",
		"QEMU",
		"440FX - 82441FX PMC",
	},
	{
		PCI_VENDOR_INTEL,
		0x7000,
		0x1AF4,
		0x1100,
		"Intel",
		"82371SB PIIX3 ISA [Natoma/Triton II]",
		"QEMU",
		"82371SB PIIX3 ISA [Natoma/Triton II]",
	},
	{
		PCI_VENDOR_INTEL,
		0x7192,
		0x15AD,
		0,
		"Intel",
		"440BX/ZX/DX - 82443BX/ZX/DX Host bridge (AGP Disabled)",
		"VMware",
		0,
	},
	{
		PCI_VENDOR_INTEL,
		0x7191,
		0, 0,
		"Intel",
		"440BX/ZX/DX - 82443BX/ZX/DX AGP bridge",
		0, 0,
	},
	{
		PCI_VENDOR_INTEL,
		0x7110,
		0x15AD,
		0,
		"Intel",
		"82371AB/EB/MB PIIX4 ISA",
		"VMware",
		0,
	},
	{
		PCI_VENDOR_INTEL,
		0x7111,
		0x15AD,
		0,
		"Intel",
		"82371AB/EB/MB PIIX4 IDE",
		"VMware",
		"VMware 82371AB/EB/MB PIIX4 IDE",
	},
	{
		PCI_VENDOR_VMWARE,
		0x0790,
		0, 0,
		"VMware",
		"PCI bridge",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1485,
		0, 0,
		"AMD",
		"Starship/Matisse Reserved SPP (System Platform Processor)",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x148A,
		0, 0,
		"AMD",
		"Starship/Matisse PCIe Dummy Function",
		0, 0,
	},
	{
		PCI_VENDOR_KIOXIA,
		0x0001,
		PCI_VENDOR_KIOXIA,
		0x0001,
		"KIOXIA",
		"NVMe SSD Controller BG4 (DRAM-less)",
		"KIOXIA",
		"NVMe SSD Controller BG4 (DRAM-less)",
	},
	{
		PCI_VENDOR_MEDIATEK,
		0x0616,
		PCI_VENDOR_MEDIATEK,
		0x0616,
		"MediaTek",
		"MT7922 802.11ax PCI Express Wireless Network Adapter",
		"MediaTek",
		"MT7922 802.11ax PCI Express Wireless Network Adapter",
	},
	{
		PCI_VENDOR_AMD,
		0x43EA,
		0, 0,
		"AMD",
		"500 Series Chipset Switch Downstream Port",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x43E9,
		0, 0,
		"AMD",
		"500 Series Chipset Switch Upstream Port",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x43EE,
		0x1B21,
		0x1142,
		"AMD",
		"500 Series Chipset USB 3.1 XHCI Controller",
		"ASMedia",
		"ASM1042A USB 3.0 Host Controller",
	},
	{
		PCI_VENDOR_AMD,
		0x43EE,
		0x1B21,
		0x1062,
		"AMD",
		"500 Series Chipset SATA Controller",
		"ASMedia",
		"ASM1062 Serial ATA Controller",
	},
	{
		PCI_VENDOR_AMD,
		0x1440,
		0, 0,
		"AMD",
		"Matisse/Vermeer Data Fabric: Device 18h; Function 0",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1441,
		0, 0,
		"AMD",
		"Matisse/Vermeer Data Fabric: Device 18h; Function 1",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1442,
		0, 0,
		"AMD",
		"Matisse/Vermeer Data Fabric: Device 18h; Function 2",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1443,
		0, 0,
		"AMD",
		"Matisse/Vermeer Data Fabric: Device 18h; Function 3",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1444,
		0, 0,
		"AMD",
		"Matisse/Vermeer Data Fabric: Device 18h; Function 4",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1445,
		0, 0,
		"AMD",
		"Matisse/Vermeer Data Fabric: Device 18h; Function 5",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1446,
		0, 0,
		"AMD",
		"Matisse/Vermeer Data Fabric: Device 18h; Function 6",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1447,
		0, 0,
		"AMD",
		"Matisse/Vermeer Data Fabric: Device 18h; Function 7",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1482,
		0, 0,
		"AMD",
		"Starship/Matisse PCIe Dummy Host Bridge",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x1483,
		0x01DE,
		0xFFF9,
		"AMD",
		"Starship/Matisse GPP Bridge",
		"Gimlet/Cosmo",
		"Baseboard",
	},
	{
		PCI_VENDOR_AMD,
		0x1484,
		0, 0,
		"AMD",
		"Starship/Matisse Internal PCIe GPP Bridge 0 to bus",
		0, 0,
	},
	{
		PCI_VENDOR_AMD,
		0x790B,
		0x1462,
		0x7C91,
		"AMD",
		"FCH SMBus Controller",
		"MSI", 
		"MS-7C91 FCH SMBus Controller",
	},
	{
		PCI_VENDOR_AMD,
		0x1480,
		0x1462,
		0x7C91,
		"AMD",
		"Starship/Matisse Root Complex",
		"MSI", 
		"MS-7C91 Starship/Matisse Root Complex",
	},
	{
		PCI_VENDOR_INTEL,
		0x7111,
		PCI_VENDOR_EMU,
		0x8263,
		"Intel",
		"82371AB/EB/MB PIIX4 IDE",
		"Emulator",
		"Emulator PIIX4 IDE",
	},
	{
		PCI_VENDOR_EMU,
		0x4444,
		PCI_VENDOR_EMU,
		0x4444,
		"Emulator",
		"Basic VESA Graphical GPU",
		"Emulator",
		"Basic VESA Graphical GPU",
	},
	{
		PCI_VENDOR_EMU,
		0x5244,
		PCI_VENDOR_EMU,
		0x5244,
		"Emulator",
		"Basic VGA Text GPU",
		"Emulator",
		"Basic VGA Text GPU",
	},
	{
		PCI_VENDOR_EMU,
		0x8263,
		PCI_VENDOR_EMU,
		0x8263,
		"Emulator",
		"HDD ATA PIO",
		"Emulator",
		"HDD ATA PIO",
	},
	{0},
};

void pci_show_device_info(byte bus, byte slot, byte func) {
	uint32 class = pci_read_config(bus, slot, func, 0x08);

	byte category = (class >> 24) & 0xFF;
	byte subclass = (class >> 16) & 0xFF;

	const byte* name = "Unknown";

	for (size_t i = 0; device_names[i].name; i++) {
		if (device_names[i].category == category &&
			device_names[i].subclass == subclass) {
			name = device_names[i].name;

			break;
		}
	}

	uint32 id = 		pci_read_config(bus, slot, func, PCI_CONFIG_VENDOR_ID);

	uint32 vendor_id = (id) 		& 0xFFFF;
	uint32 device_id = (id >> 16) 	& 0xFFFF;

	uint32 subsys_id = 	pci_read_config(bus, slot, func, PCI_CONFIG_SUBSYS_ID);

	uint32 subsys_vendor_id = (subsys_id) 			& 0xFFFF;
	uint32 subsys_device_id = (subsys_id >> 16) 	& 0xFFFF;

	const byte* vendor_name = 			"Unknown";
	const byte* device_name = 			"Unknown";

	const byte* subsys_vendor_name = 	"Unknown";
	const byte* subsys_device_name = 	"Unknown";

	bool vendor_available = vendor_names[0].vendor_name != 0;
	bool device_available = vendor_names[0].device_name != 0;

	bool subsys_vendor_available = vendor_names[0].subsys_vendor_name != 0;
	bool subsys_device_available = vendor_names[0].subsys_device_name != 0;

	bool ok = 	vendor_available 		|| device_available || 
				subsys_vendor_available || subsys_device_available;

	for (size_t i = 0; ok; i++) {
		bool vendor_found = vendor_names[i].vendor == vendor_id;
		bool device_found = vendor_names[i].device == device_id;

		bool subsys_vendor_found = vendor_names[i].subsys_vendor == subsys_vendor_id;
		bool subsys_device_found = vendor_names[i].subsys_device == subsys_device_id;

		vendor_available = vendor_names[i].vendor_name != 0;
		device_available = vendor_names[i].device_name != 0;

		subsys_vendor_available = vendor_names[i].subsys_vendor_name != 0;
		subsys_device_available = vendor_names[i].subsys_device_name != 0;

		if (vendor_available && vendor_found) {
			if (device_available && device_found) {
				if (subsys_vendor_available && subsys_vendor_found) {
					if (subsys_device_available && subsys_device_found) {
						subsys_device_name = vendor_names[i].subsys_device_name;
					}

					subsys_vendor_name = vendor_names[i].subsys_vendor_name;
				}

				device_name = vendor_names[i].device_name;
			}

			vendor_name = vendor_names[i].vendor_name;
		}

		ok = 	vendor_available 		|| device_available || 
				subsys_vendor_available || subsys_device_available;
		
		if (!ok) break;
	}

	byte* info_categories[6] = {
		"Device type",
		"Vendor",
		"Device",
		"Subsystem vendor",
		"Subsystem device",
		"Location",
	};

	byte data[6][64] = { "Unknown" };

	size_t info_categories_cnt = sizeof(info_categories) / sizeof(info_categories[0]);

	size_t data_cnt = sizeof(data) / sizeof(data[0]);

	size_t info_cnt = MIN(info_categories_cnt, data_cnt);

	snprintf(data[0], 64, "%s (0x%.2x, 0x%.2x)", name, category, subclass);
	snprintf(data[1], 64, "%s (0x%.4x)", vendor_name, vendor_id);
	snprintf(data[2], 64, "%s (0x%.4x)", device_name, device_id);
	snprintf(data[3], 64, "%s (0x%.4x)", subsys_vendor_name, subsys_vendor_id);
	snprintf(data[4], 64, "%s (0x%.4x)", subsys_device_name, subsys_device_id);
	snprintf(data[5], 64, "0x%.2x:0x%.2x:0x%.2x", bus, slot, func);

	ssize_t info_category_max_len = 0;

	ssize_t info_max_len = 0;

	for (ssize_t i = 0; i < info_cnt; i++) {
		byte* info_category = info_categories[i];

		ssize_t info_category_len = strlen(info_category);

		if (info_category_len > info_category_max_len) {
			info_category_max_len = info_category_len;
		}

		byte* info_data = data[i];

		ssize_t info_data_len = strlen(info_data);

		if (info_data_len > info_max_len) {
			info_max_len = info_data_len;
		}
	}

	ssize_t table_width = 1 + 1 + info_category_max_len + 1 + 1 + info_max_len + 1 + 1;

	ssize_t old_info_max_len = info_max_len;

	size_t columns = get_columns();

	size_t rows = get_rows();

	if (table_width >= columns) {
		info_max_len -= table_width - columns + 2;
	}

	info_category_max_len += 2;

	info_max_len += 2;

	ssize_t center = (columns / 2) - (info_category_max_len + 1 + info_max_len) / 2;

	kprintf("%*s┌%0m─*s┐\n", center, "", info_category_max_len + 1 + info_max_len, "");

	kprintf("%*s│%=*s│\n", center, "", info_category_max_len + 1 + info_max_len, data[0]);

	kprintf("%*s├%0m─*s┬%0m─*s┤\n", center, "", info_category_max_len, "", info_max_len, "");

	for (size_t i = 1; i < info_cnt; i++) {
		byte* info_category = info_categories[i];

		byte* info_data = data[i];

		bool dots = (1 + 1 + info_category_max_len + 1 + 1 + strlen(info_data) + 1 + 1) >= columns;

		kprintf("%*s│ %vfby%-*...*s%vd │ %vfbg%*...*s%vd │\n", center, "", info_category_max_len - 2, info_category_max_len - 2, info_category, info_max_len - 2, info_max_len - 2, info_data);
	}

	kprintf("%*s└%0m─*s┴%0m─*s┘\n", center, "", info_category_max_len, "", info_max_len, "");
}

void pci_scan(void) {
	size_t columns = get_columns();

	size_t pci_devices_cnt = 0;

	for (size_t bus = 0; bus < 256; bus++) {
		for (size_t slot = 0; slot < 32; slot++) {
			for (size_t func = 0; func < 8; func++) {
				uint32 data = pci_read_config(bus, slot, func, 0x00) & 0xFFFF;

				if (data == 0xFFFF) break;

				pci_show_device_info(bus, slot, func);

				pci_devices_cnt += 1;

				kprintf("%=*s\n", columns, "Press any key to continue...");

				blkgetch();
			}
		}
	}

	byte buf[64] = { 0 };

	size_t writed = snprintf(buf, 64, "PCI devices count: %zu", pci_devices_cnt);

	size_t center = (columns / 2) - (writed / 2);

	kprintf("%vfbg%0m=*s%vd%.*s%vfbg%0m=*s%vd\n", center, "", writed, buf, center, "");
}
