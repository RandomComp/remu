#include "multiboot.h"

#include "std.h"

void display_multiboot(void* ram, uint32 magic, multiboot_info_t* multiboot) {
	bool is_fb_available = (multiboot->flags & 0x1000) != 0;

	bool is_vbe_available = (multiboot->flags & 0x800) != 0;

	bool is_apm_available = (multiboot->flags & 0x400) != 0;

	bool is_bootloader_name_available = (multiboot->flags & 0x200) != 0;

	bool is_config_table_available = (multiboot->flags & 0x100) != 0;

	bool is_drivers_available = (multiboot->flags & 0x80) != 0;

	bool is_mmap_available = (multiboot->flags & 0x40) != 0;

	bool is_cmdline_available = (multiboot->flags & 0x04) != 0;

	bool is_boot_device_available = (multiboot->flags & 0x02) != 0;

	bool is_mem_available = (multiboot->flags & 0x01) != 0;

	kprintf("multiboot magic = %x\n\r", magic);

	kprintf("multiboot->flags = %b\n\r", multiboot->flags);
	
	if (is_mem_available) {
		kprintf("multiboot->mem_lower = %x\n\r", multiboot->mem_lower);
		
		kprintf("multiboot->mem_upper = %x\n\r", multiboot->mem_upper);
	}
	
	if (is_boot_device_available) {
		kprintf("multiboot->boot_device = %x\n\r", multiboot->boot_device);
	}
	
	if (is_cmdline_available) {
		kprintf("multiboot->cmdline = %s\n\r", ram + multiboot->cmdline);
	}

	if (is_mmap_available) {
		kprintf("multiboot->mmap_length = %x\n\r", multiboot->mmap_length);

		kprintf("multiboot->mmap_addr = %x\n\r", multiboot->mmap_addr);
	}

	if (is_drivers_available) {
		kprint("multiboot->drives_length = 0x"); print_num(multiboot->drives_length, 16, false, false); kprint("\n\r");
		kprint("multiboot->drives_addr = 0x"); print_num(multiboot->drives_addr, 16, false, false); kprint("\n\r");
	}

	if (is_config_table_available) {
		kprint("multiboot->config_table = "); print_num(multiboot->config_table, 10, false, false); kprint("\n\r");
	}

	if (is_bootloader_name_available) {
		kprintf("multiboot->boot_loader_name = %s\n\r", ram + multiboot->boot_loader_name);
	}

	if (is_apm_available) {
		kprint("multiboot->apm_table = "); print_num(multiboot->apm_table, 10, false, false); kprint("\n\r");
	}

	if  (is_vbe_available) {
		kprint("multiboot->vbe_control_info = "); print_num(multiboot->vbe_control_info, 10, false, false); kprint("\n\r");
		kprint("multiboot->vbe_mode_info = "); print_num(multiboot->vbe_mode_info, 10, false, false); kprint("\n\r");
		kprint("multiboot->vbe_mode = "); print_num(multiboot->vbe_mode, 10, false, false); kprint("\n\r");
		kprint("multiboot->vbe_interface_seg = "); print_num(multiboot->vbe_interface_seg, 10, false, false); kprint("\n\r");
		kprint("multiboot->vbe_interface_off = "); print_num(multiboot->vbe_interface_off, 10, false, false); kprint("\n\r");
		kprint("multiboot->vbe_interface_len = "); print_num(multiboot->vbe_interface_len, 10, false, false); kprint("\n\r");
	}

	if (is_fb_available) {
		kprintf("multiboot->fb_addr = %x\n\r", multiboot->fb_addr);
		kprint("multiboot->fb_pitch = "); print_num(multiboot->fb_pitch, 10, false, false); kprint("\n\r");
		kprint("multiboot->fb_size = "); print_num(multiboot->fb_width, 10, false, false); putch('x');
		print_num(multiboot->fb_height, 10, false, false); putch('x'); 
		print_num(multiboot->fb_bpp, 10, false, false); kprint("\n\r");
		kprint("multiboot->fb_type = "); print_num(multiboot->fb_type, 10, false, false); kprint("\n\r");
	}
}
