#include <setjmp.h>
#include "EAF/filber/internal/stack.h"

eaf_jmp_buf_t* eaf_stack_calculate_jmpbuf(void* addr, size_t size)
{
	return (eaf_jmp_buf_t*)((char*)addr + size - sizeof(jmp_buf));
}
