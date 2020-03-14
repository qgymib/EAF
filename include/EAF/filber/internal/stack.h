#ifndef __EAF_FILBER_INTERNAL_STACK_H__
#define __EAF_FILBER_INTERNAL_STACK_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

struct eaf_jmpbuf;
struct eaf_jmpbuf* eaf_stack_calculate_jmpbuf(void* addr, size_t size);
extern void eaf_asm_stackcall(struct eaf_jmpbuf* jmp, void(*fn)(void*), void* arg);

#ifdef __cplusplus
}
#endif
#endif
