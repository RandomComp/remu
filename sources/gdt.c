#include "gdt.h"

#include "types.h"

#ifndef __EMULATOR__
extern void GDTFlush(uint32);
#endif

static struct gdt_entry_struct gdt_entries[5];

static struct gdt_ptr_struct gdt_ptr;

static bool bGDTInitialized = false;

void gdt_init(void) {
	#ifdef __EMULATOR__
	return;
	#endif

	if (bGDTInitialized) return;

	gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 5) - 1;

	gdt_ptr.base = (size_t)(&gdt_entries);

	GDTSetGate(0, 0, 0, 0, 0);
	GDTSetGate(1, 0, 0xffffffff, 0x9a, 0xcf);
	GDTSetGate(2, 0, 0xffffffff, 0x92, 0xcf);
	GDTSetGate(3, 0, 0xffffffff, 0xfa, 0xcf);
	GDTSetGate(4, 0, 0xffffffff, 0xf2, 0xcf);

	#ifndef __EMULATOR__
	GDTFlush((size_t)(&gdt_ptr));
	#endif

	bGDTInitialized = true;
}

void GDTSetGate(uint32 num, uint32 base, uint32 limit, uint8 access, uint8 gran) {
	gdt_entries[num].base_low = (base & 0xffff);
	gdt_entries[num].base_middle = (base >> 16) & 0xff;
	gdt_entries[num].base_high = (base >> 24) & 0xff;

	gdt_entries[num].limit = (limit & 0xffff);

	gdt_entries[num].flags = (limit >> 16) & 0x0f;
	gdt_entries[num].flags |= (gran & 0xf0);

	gdt_entries[num].access = access;
}
