.text

.global loader

.set MAGIC, 0x1badb002

.set FLAGS, 0b00000111

.set CHECKSUM, -(MAGIC + FLAGS)

.align 4

	.long MAGIC

	.long FLAGS

	.long CHECKSUM

	.long 0, 0, 0, 0, 0
	
	.long 0
	.long 1024
	.long 768
	.long 32

.set stackSize, 0x2000

.lcomm stack, stackSize

.comm mbd, 4

.comm magic, 4

loader:
	movl $(stack + stackSize), %esp
	
	push %ebx

	push %eax

	call kmain

	cli
hang:
	hlt

	jmp hang

.section .note.GNU-stack,"",@progbits
