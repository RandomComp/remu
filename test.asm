section .data
	

section .text
	global printf
	global exit123

; TODO: Реализовать свою минимальную обертку над системными вызовами linux, с поддержкой:
; printf, ioctl, tcgetattr, atexit, tcsetattr, fcntl, setbuf, signal, sigemptyset, sigaction, timer_create, timer_settime
; malloc, memcpy, free, open, read, write, exit, 

printf:
	push rdx
	push rax

	push rsi

	mov rsi, rdi

	pop rdx

	mov rax, 1
	mov rdi, 1
	;mov rsi, msg
	;mov rdx, len
	syscall

	pop rax
	pop rdx

	ret

exit123:
	push rax

	mov rax, 60
	syscall

	pop rax

	ret