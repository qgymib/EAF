#include "EAF/filber/internal/jmpbuf.h"
#include "arch/eaf_setjmp.h"

size_t eaf_jmpbuf_size(void)
{
	return sizeof(eaf_jmpbuf_t);
}

struct eaf_jmpbuf* eaf_stack_calculate_jmpbuf(void* addr, size_t size)
{
	return (struct eaf_jmpbuf*)((char*)addr + size - eaf_jmpbuf_size());
}
