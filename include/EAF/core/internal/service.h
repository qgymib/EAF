#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <setjmp.h>
#include "EAF/utils/define.h"

#if defined(_MSC_VER)
#	define EAF_FILBER_YIELD_TOKEN	__COUNTER__ + 1
#else
#	define EAF_FILBER_YIELD_TOKEN	__LINE__
#endif

#define EAF_FILBER_REENTER()	\
	eaf_filber_local_t* _eaf_local = eaf_filber_get_local();\
	switch(_eaf_local->branch)\
		case (unsigned)-1: if (_eaf_local->branch)\
		{\
			goto terminate_coroutine;\
		terminate_coroutine:\
			_eaf_local->branch = (unsigned)-1;\
			goto bail_out_of_coroutine;\
		bail_out_of_coroutine:\
			break;\
		}\
		else /* fall-through */ case 0:

#define EAF_FILBER_YIELD(n)	\
	for (_eaf_local->branch = (n), _eaf_local->cc[0] = EAF_COROUTINE_CC0_YIELD;;)\
		if (_eaf_local->branch == 0) {\
			case (n): ;\
			break;\
		} else\
		switch (_eaf_local->branch ? 0 : 1)\
			for(;;)\
				/* fall-through */ case -1: if (_eaf_local->branch)\
					goto terminate_coroutine;\
				else for (;;)\
					/* fall-through */ case 1: if (_eaf_local->branch)\
					goto bail_out_of_coroutine;\
				else /* fall-through */ case 0: { }

#define EAF_COROUTINE_CC0_YIELD		(0x01 << 0x00)	/** yield */

typedef struct eaf_filber_local
{
	unsigned	branch;		/** yield branch */
	unsigned	cc[1];		/** coroutine control */
}eaf_filber_local_t;

/**
* 获取本地数据
* @return		eaf_filber_local_t*
*/
eaf_filber_local_t* eaf_filber_get_local(void);

#ifdef __cplusplus
}
#endif
#endif
