.syntax unified
.global eaf_asm_stackcall
.type eaf_asm_stackcall,%function
eaf_asm_stackcall:
	mov		r7,	r0			/* 将jmp保存至持久性寄存器 */
	sub		sp,	r0,	#0x8	/* 构建堆栈 */
	mov		r0,	r2			/* 调用用户函数 */
	blx		r1
	mov		r0,	r7			/* 还原用户环境 */
	b		longjmp
