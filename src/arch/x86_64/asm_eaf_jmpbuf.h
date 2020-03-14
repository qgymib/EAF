#ifndef __EAF_ARCH_X86_64_ASM_EAF_JUMPBUF_INTERNAL_H__
#define __EAF_ARCH_X86_64_ASM_EAF_JUMPBUF_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/*******************************************************
*                                                     *
*  -------------------------------------------------  *
*  |  00 |  01 |  02 |  03 |  04 |  05 |  06 |  07 |  *
*  -------------------------------------------------  *
*  | 0x00| 0x04| 0x08| 0x0c| 0x10| 0x14| 0x18| 0x1c|  *
*  -------------------------------------------------  *
*  |    rbx    |    rbp    |    r12    |    r13    |  *
*  -------------------------------------------------  *
*  -------------------------------------------------  *
*  |  08 |  09 |  10 |  11 |  12 |  13 |  14 |  15 |  *
*  -------------------------------------------------  *
*  | 0x20| 0x24| 0x28| 0x2c| 0x30| 0x34| 0x38| 0x3c|  *
*  -------------------------------------------------  *
*  |    r14    |    r15    |    rsp    |     fp    |  *
*  -------------------------------------------------  *
*                                                     *
*******************************************************/
struct eaf_jmpbuf
{
	unsigned long buf[8];
};

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif	/* __EAF_ARCH_X86_64_ASM_EAF_JUMPBUF_INTERNAL_H__ */
