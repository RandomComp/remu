#ifndef _EMULATOR_OS_IDT_H
#define _EMULATOR_OS_IDT_H

#include "types.h"

typedef struct PACKED idt_entry_t {
	#ifndef __EMULATOR__
	uint16 base_low;
	#else
	uint32 base_low;
	#endif

	uint16 sel;

	uint8 always0;

	uint8 flags;

	#ifndef __EMULATOR__
	uint16 base_high;
	#else
	uint32 base_high;
	#endif
} idt_entry_t;

typedef struct PACKED idt_ptr_t {
	uint16 limit;

	#ifndef __EMULATOR__
	uint32 base;
	#else
	uint64 base;
	#endif
} idt_ptr_t;

typedef struct registers_t {
	uint32 cr2;

	uint32 ds;

	uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax;

	uint32 int_no, err_code;

	uint32 eip, csm, eflags, useresp, ss;
} registers_t;

typedef void (*isr_handler_t)(registers_t* regs);

void idt_init(void);

#ifndef __EMULATOR__
void IDTSetGate(uint8 num, uint32 base, uint16 sel, uint8 flags);
#else
void IDTSetGate(uint8 num, uint64 base, uint16 sel, uint8 flags);
#endif

void IDTISRHandler(struct registers_t* regs);

void IDTIRQInstallHandler(int32 irq, void (*handler)(struct registers_t* regs));

void IDTIRQUninstallHandler(int32 irq);

void IDTIRQHandler(struct registers_t* regs);

#ifndef __EMULATOR__
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void isr17(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

extern void isr128(void);
extern void isr177(void);
#else
void isr0(void);
void isr1(void);
void isr2(void);
void isr3(void);
void isr4(void);
void isr5(void);
void isr6(void);
void isr7(void);
void isr8(void);
void isr9(void);
void isr10(void);
void isr11(void);
void isr12(void);
void isr13(void);
void isr14(void);
void isr15(void);
void isr16(void);
void isr17(void);
void isr18(void);
void isr19(void);
void isr20(void);
void isr21(void);
void isr22(void);
void isr23(void);
void isr24(void);
void isr25(void);
void isr26(void);
void isr27(void);
void isr28(void);
void isr29(void);
void isr30(void);
void isr31(void);

void irq0(void);
void irq1(void);
void irq2(void);
void irq3(void);
void irq4(void);
void irq5(void);
void irq6(void);
void irq7(void);
void irq8(void);
void irq9(void);
void irq10(void);
void irq11(void);
void irq12(void);
void irq13(void);
void irq14(void);
void irq15(void);

void isr128(void);
void isr177(void);
#endif
#endif
