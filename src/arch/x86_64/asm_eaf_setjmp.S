.global eaf_asm_setjmp
.type eaf_asm_setjmp,@function
eaf_asm_setjmp:
	mov		%rbx,		(%rdi)			/* rdi is jmp_buf, move registers onto it */
	mov		%rbp,		8(%rdi)
	mov		%r12,		16(%rdi)
	mov		%r13,		24(%rdi)
	mov		%r14,		32(%rdi)
	mov		%r15,		40(%rdi)
	lea		8(%rsp),	%rdx			/* this is our rsp WITHOUT current ret addr */
	mov		%rdx,		48(%rdi)
	mov		(%rsp),		%rdx			/* save return addr ptr for new rip */
	mov		%rdx,		56(%rdi)
	mov		8(%rbp),	%rax			/* save orig return addr */
	mov		%rax,		64(%rdi)
	xor		%rax,		%rax			/* always return 0 */
	ret
