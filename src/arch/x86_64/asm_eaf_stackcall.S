.global eaf_asm_stackcall
.type eaf_asm_stackcall,@function
eaf_asm_stackcall:
	mov		%rdi,	%r15	/* %r15在调用间保证不变 */
	mov		%rdx,	%rdi

	mov		%r15,	%rsp	/* 构造栈 */
	subq	$0x8,	%rsp
	callq	*%rsi

	mov		%r15,	%rdi	/* 此处栈可能被破坏，由于通过寄存器传参，可以忽略 */
	mov		%r15,	%rsi
	jmp		eaf_asm_longjmp
