#ifndef __EAF_ERRNO_H__
#define __EAF_ERRNO_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum eaf_errno
{
	eaf_errno_success		=  0x00,	/** 成功 */
	eaf_errno_unknown		= -0x01,	/** 未知错误 */
	eaf_errno_duplicate		= -0x02,	/** 重复操作 */
	eaf_errno_memory		= -0x03,	/** 内存异常 */
	eaf_errno_state			= -0x04,	/** 状态错误 */
	eaf_errno_notfound		= -0x05,	/** 资源未找到 */
	eaf_errno_overflow		= -0x06,	/** 溢出 */
	eaf_errno_timeout		= -0x07,	/** 超时 */
}eaf_errno_t;

/**
* 获取描述自定错误码的字符串
* @param err	错误码
* @return		描述
*/
const char* eaf_strerror(eaf_errno_t err);

#ifdef __cplusplus
}
#endif
#endif
