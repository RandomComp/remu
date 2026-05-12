#include "idt.h"

#include "gdt.h"

#include "types.h"

#include "std/stdio.h"

#include "drivers/io.h"

#include "builtins/string.h"

#include "builtins/builtins.h"

#ifndef __EMULATOR__
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

#ifdef __EMULATOR__
// def idt_routine_call(is_isr, _int):
// 	if not is_isr:
// 		_int -= 0x20
// 	print("\nvoid " + ("isr" if is_isr else "irq") + str(_int) + "(void) {")
// 	if not is_isr:
// 		print("""\tisr_handler_t handler = (isr_handler_t)IDTIRQRoutines[""" + str(_int) + """];
// \tif (handler) handler();""")
// 	else:
// 		print("\tcall_isr_handler(" + str(_int) + ");")
// 	print("}")

void call_isr_handler(byte isr_int) {
	kprintf("\nError %i\n", isr_int);

	for (;;) halt();
}

void isr0(void) {
	call_isr_handler(0);
}

void isr1(void) {
	call_isr_handler(1);
}

void isr2(void) {
	call_isr_handler(2);
}

void isr3(void) {
	call_isr_handler(3);
}

void isr4(void) {
	call_isr_handler(4);
}

void isr5(void) {
	call_isr_handler(5);
}

void isr6(void) {
	call_isr_handler(6);
}

void isr7(void) {
	call_isr_handler(7);
}

void isr8(void) {
	call_isr_handler(8);
}

void isr9(void) {
	call_isr_handler(9);
}

void isr10(void) {
	call_isr_handler(10);
}

void isr11(void) {
	call_isr_handler(11);
}

void isr12(void) {
	call_isr_handler(12);
}

void isr13(void) {
	call_isr_handler(13);
}

void isr14(void) {
	call_isr_handler(14);
}

void isr15(void) {
	call_isr_handler(15);
}

void isr16(void) {
	call_isr_handler(16);
}

void isr17(void) {
	call_isr_handler(17);
}

void isr18(void) {
	call_isr_handler(18);
}

void isr19(void) {
	call_isr_handler(19);
}

void isr20(void) {
	call_isr_handler(20);
}

void isr21(void) {
	call_isr_handler(21);
}

void isr22(void) {
	call_isr_handler(22);
}

void isr23(void) {
	call_isr_handler(23);
}

void isr24(void) {
	call_isr_handler(24);
}

void isr25(void) {
	call_isr_handler(25);
}

void isr26(void) {
	call_isr_handler(26);
}

void isr27(void) {
	call_isr_handler(27);
}

void isr28(void) {
	call_isr_handler(28);
}

void isr29(void) {
	call_isr_handler(29);
}

void isr30(void) {
	call_isr_handler(30);
}

void isr31(void) {
	call_isr_handler(31);
}

void isr128(void) {
	call_isr_handler(128);
}

void isr177(void) {
	call_isr_handler(177);
}

// TODO: ╨┐╨╡╤Ç╨╡╨┤╨░╨▓╨░╤é╤î ╨▓ handler ╤Ç╨╡╨░╨╗╤î╨╜╤ï╨╣ struct registers_t* ╨░ ╨╜╨╡ ╨┐╤â╤ü╤é╨╛╨╡ ╨╖╨╜╨░╤ç╨╡╨╜╨╕╨╡
void irq0(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[0];
	if (handler) handler(nullptr);
}

void irq1(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[1];
	if (handler) handler(nullptr);
}

void irq2(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[2];
	if (handler) handler(nullptr);
}

void irq3(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[3];
	if (handler) handler(nullptr);
}

void irq4(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[4];
	if (handler) handler(nullptr);
}

void irq5(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[5];
	if (handler) handler(nullptr);
}

void irq6(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[6];
	if (handler) handler(nullptr);
}

void irq7(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[7];
	if (handler) handler(nullptr);
}

void irq8(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[8];
	if (handler) handler(nullptr);
}

void irq9(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[9];
	if (handler) handler(nullptr);
}

void irq10(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[10];
	if (handler) handler(nullptr);
}

void irq11(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[11];
	if (handler) handler(nullptr);
}

void irq12(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[12];
	if (handler) handler(nullptr);
}

void irq13(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[13];
	if (handler) handler(nullptr);
}

void irq14(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[14];
	if (handler) handler(nullptr);
}

void irq15(void) {
	isr_handler_t handler = (isr_handler_t)IDTIRQRoutines[15];
	if (handler) handler(nullptr);
}

#endif

void idt_init(void) {
	if (bIDTInitialized) return;

	gdt_init();

	idtPtr.limit = sizeof(idt_entry_t) * 256 - 1;

	#ifndef __EMULATOR__
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

	void* ints[] = {
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

	#ifndef __EMULATOR__
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

#ifndef __EMULATOR__
void IDTSetGate(uint8 num, uint32 base, uint16 sel, uint8 flags)
#else
void IDTSetGate(uint8 num, uint64 base, uint16 sel, uint8 flags)
#endif
{
	#ifndef __EMULATOR__
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
	isr_handler_t handler = IDTIRQRoutines[regs->int_no - 32];

	if (handler)
		handler(regs);

	if (regs->int_no >= 40)
		out8(0xa0, 0x20);
	
	out8(0x20, 0x20);
}
