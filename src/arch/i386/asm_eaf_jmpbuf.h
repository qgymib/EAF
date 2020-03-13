#ifndef __EAF_ARCH_I386_ASM_EAF_JUMPBUF_INTERNAL_H__
#define __EAF_ARCH_I386_ASM_EAF_JUMPBUF_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/*******************************************************
*                                                     *
*  -------------------------------------------------  *
*  |  00 |  01 |  02 |  03 |  04 |  05 |  06 |     |  *
*  -------------------------------------------------  *
*  | 0x00| 0x04| 0x08| 0x0c| 0x10| 0x14| 0x18|     |  *
*  -------------------------------------------------  *
*  | ebx | esi | edi | ebp | esp |  fp | RET |     |  *
*  -------------------------------------------------  *
*                                                     *
*******************************************************/
struct eaf_jmpbuf
{
	unsigned long buf[7];
};

#ifdef __cplusplus
};
#endif	/* __cplusplus */
#endif	/* __EAF_ARCH_I386_ASM_EAF_JUMPBUF_INTERNAL_H__ */
