.global eaf_asm_stackcall
.type eaf_asm_stackcall,%function
eaf_asm_stackcall:
	mov		x19,	x0
	sub		sp,		x0,	#0x8
	mov		x0,		x2
	blr		x1
	mov		x0,		x19
	b		longjmp
