#include "idt.h"

#include "gdt.h"

#include "types.h"

#include "std.h"

#include "drivers/memory/memory.h"

#include "builtins/string.h"

#include "builtins/builtins.h"

#ifdef FREE_STANDING_MODE
extern void IDTFlush(uint32);
#else
#include "kernel.h"

extern __init_kernel_args_t kernel_args;
#endif

static idt_entry_t idt_entries[256];

static idt_ptr_t idtPtr;

static bool bIDTInitialized = false;

static void* IDTIRQRoutines[16] = {
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,
};

void print_something(void) {
	kprintf("Ok...\n");
}

#ifndef FREE_STANDING_MODE
void isr0(void) {
	// kprintf("isr123232\n");
}

void isr1(void) {
	kprintf("isr1\n");
}

void isr2(void) {
	kprintf("isr2\n");
}

void isr3(void) {
	kprintf("isr3\n");
}

void isr4(void) {
	kprintf("isr4\n");
}

void isr5(void) {
	kprintf("isr5\n");
}

void isr6(void) {
	kprintf("isr6\n");
}

void isr7(void) {
	kprintf("isr7\n");
}

void isr8(void) {
	kprintf("isr8\n");
}

void isr9(void) {
	kprintf("isr9\n");
}

void isr10(void) {
	kprintf("isr10\n");
}

void isr11(void) {
	kprintf("isr11\n");
}

void isr12(void) {
	kprintf("isr12\n");
}

void isr13(void) {
	kprintf("isr13\n");
}

void isr14(void) {
	kprintf("isr14\n");
}

void isr15(void) {
	kprintf("isr15\n");
}

void isr16(void) {
	kprintf("isr16\n");
}

void isr17(void) {
	kprintf("isr17\n");
}

void isr18(void) {
	kprintf("isr18\n");
}

void isr19(void) {
	kprintf("isr19\n");
}

void isr20(void) {
	kprintf("isr20\n");
}

void isr21(void) {
	kprintf("isr21\n");
}

void isr22(void) {
	kprintf("isr22\n");
}

void isr23(void) {
	kprintf("isr23\n");
}

void isr24(void) {
	kprintf("isr24\n");
}

void isr25(void) {
	kprintf("isr25\n");
}

void isr26(void) {
	kprintf("isr16\n");
}

void isr27(void) {
	kprintf("isr27\n");
}

void isr28(void) {
	kprintf("isr28\n");
}

void isr29(void) {
	kprintf("isr29\n");
}

void isr30(void) {
	kprintf("isr30\n");
}

void isr31(void) {
	kprintf("isr31\n");
}

static uint64 pit_time = 0;

void irq0(void) {
	// void (*handler)(void);

	// handler = IDTIRQRoutines[0];

	// if (handler)
	// 	handler();

	// kprintf("\rPIT interrupt! time: %l", pit_time);

	pit_time += 18;
}

void irq1(void) {
	void (*handler)(void);

	handler = IDTIRQRoutines[1];

	if (handler)
		handler();
		
	kprintf("Keyboard interrupt!\n");
}

void irq2(void) {
	kprintf("irq2\n");
}

void irq3(void) {
	kprintf("irq3\n");
}

void irq4(void) {
	kprintf("isr4\n");
}

void irq5(void) {
	kprintf("isr5\n");
}

void irq6(void) {
	kprintf("irq6\n");
}

void irq7(void) {
	kprintf("irq7\n");
}

void irq8(void) {
	kprintf("irq8\n");
}

void irq9(void) {
	kprintf("irq9\n");
}

void irq10(void) {
	kprintf("irq10\n");
}

void irq11(void) {
	kprintf("irq11\n");
}

void irq12(void) {
	kprintf("isr12\n");
}

void irq13(void) {
	kprintf("isr13\n");
}

void irq14(void) {
	kprintf("isr14\n");
}

void irq15(void) {
	kprintf("irq15\n");
}


void isr128(void) {
	kprintf("isr128\n");
}

void isr177(void) {
	kprintf("isr177\n");
}
#endif

void idt_init(void) {
	if (bIDTInitialized) return;

	gdt_init();

	idtPtr.limit = sizeof(idt_entry_t) * 256 - 1;

	#ifdef FREE_STANDING_MODE
	idtPtr.base = (uint32)&idt_entries;
	#else
	idtPtr.base = (uint64)(idt_entries);
	#endif

	memset(idt_entries, 0, sizeof(idt_entry_t) * 256);

	out8(0x20, 0x11);
	out8(0xa0, 0x11);

	out8(0x21, 0x20);
	out8(0xa1, 0x28);

	out8(0x21, 0x04);
	out8(0xa1, 0x02);

	out8(0x21, 0x01);
	out8(0xa1, 0x01);
	
	out8(0x21, 0x00);
	out8(0xa1, 0x00);

	isr_handler_t ints[] = {
		isr0, 	isr1,
		isr2, 	isr3,
		isr4, 	isr5,
		isr6, 	isr7,
		isr8, 	isr9,
		isr10, 	isr11,
		isr12, 	isr13,
		isr14, 	isr15,
		isr16, 	isr17,
		isr18, 	isr19,
		isr20, 	isr21,
		isr22, 	isr23,
		isr24, 	isr25,
		isr26, 	isr27,
		isr28, 	isr29,
		isr30, 	isr31,
		irq0, 	irq1,
		irq2, 	irq3,
		irq4, 	irq5,
		irq6, 	irq7,
		irq8, 	irq9,
		irq10, 	irq11,
		irq12, 	irq13,
		irq14, 	irq15,
	};

	size_t ints_cnt = sizeof(ints) / sizeof(ints[0]);

	#ifdef FREE_STANDING_MODE
	for (size_t i = 0; i < ints_cnt; i++) {
		IDTSetGate(i, (uint32)ints[i], 0x08, 0x8e);
	}

	IDTSetGate(128, (uint32)isr128, 0x08, 0x8e);
	IDTSetGate(177, (uint32)isr177, 0x08, 0x8e);
	
	IDTFlush((uint32)&idtPtr);
	#else
	for (size_t i = 0; i < ints_cnt; i++) {
		IDTSetGate(i, (uint64)(ints[i]), 0x08, 0x8e);
	}

	IDTSetGate(128, (uint64)isr128, 0x08, 0x8e);
	IDTSetGate(177, (uint64)isr177, 0x08, 0x8e);

	if (kernel_args.__emulator_idt_flush)
		kernel_args.__emulator_idt_flush(&idtPtr);
	#endif

	bIDTInitialized = true;
}

#ifdef FREE_STANDING_MODE
void IDTSetGate(uint8 num, uint32 base, uint16 sel, uint8 flags)
#else
void IDTSetGate(uint8 num, uint64 base, uint16 sel, uint8 flags)
#endif
{
	#ifdef FREE_STANDING_MODE
	idt_entries[num].base_low = base & 0xffff;

	idt_entries[num].base_high = (base >> 16) & 0xffff;
	#else
	idt_entries[num].base_low = base & 0xffffffff;

	idt_entries[num].base_high = (base >> 32) & 0xffffffff;
	#endif

	idt_entries[num].sel = sel;

	idt_entries[num].always0 = 0;

	idt_entries[num].flags = flags | 0x60;
}

void IDTISRHandler(struct registers_t* regs) {
	if (regs) kprintf("\nError %i\n", regs->int_no);

	for (;;) halt();
}

void IDTIRQInstallHandler(int32 irq, void (*handler)(struct registers_t* regs)) {
	IDTIRQRoutines[irq] = handler;
}

void IDTIRQUninstallHandler(int32 irq) {
	IDTIRQRoutines[irq] = 0;
}

void IDTIRQHandler(struct registers_t* regs) {
	void (*handler)(void);

	handler = IDTIRQRoutines[regs->int_no - 32];

	if (handler)
		handler();

	if (regs->int_no >= 40)
		out8(0xa0, 0x20);
	
	out8(0x20, 0x20);
}
