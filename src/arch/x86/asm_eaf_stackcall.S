.global eaf_asm_stackcall
.type eaf_asm_stackcall,@function
eaf_asm_stackcall:
	mov		0x4(%esp),		%edi	/* edi可以保证在返回之后值不变 */
	mov		0x8(%esp),		%eax	/* 用户函数 */
	mov		0xc(%esp),		%ecx	/* 用户参数 */

	mov		%edi,			%esp	/* 构造堆栈 */
	sub		$0x8,			%esp

	push	%ecx					/* 传递用户参数 */
	call	*%eax

	mov		%edi,			%esp	/* 重新构造堆栈 */
	sub		$0x8,			%esp

	push	%edi					/* 构造longjmp参数 */
	push	%edi
	jmp		eaf_asm_longjmp
