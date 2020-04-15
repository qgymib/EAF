#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "EAF/utils/define.h"

#if defined(_MSC_VER)
#	define EAF_COROUTINE_YIELD_TOKEN	__COUNTER__ + 1
#else
#	define EAF_COROUTINE_YIELD_TOKEN	__LINE__
#endif

#define EAF_COROUTINE_REENTER()	\
	eaf_group_local_t* _eaf_group_local = NULL;\
	eaf_service_local_t* _eaf_service_local = eaf_service_get_local(&_eaf_group_local);\
	switch(_eaf_service_local->branch)\
		case (unsigned)-1: if (_eaf_service_local->branch)\
		{\
			goto terminate_coroutine;\
		terminate_coroutine:\
			_eaf_service_local->branch = (unsigned)-1;\
			goto bail_out_of_coroutine;\
		bail_out_of_coroutine:\
			break;\
		}\
		else /* fall-through */ case 0:

#define EAF_COROUTINE_YIELD(_fn, _arg, n)	\
	for (_eaf_service_local->branch = (n), _eaf_group_local->cc[0] = EAF_SERVICE_CC0_YIELD,\
		_eaf_group_local->yield.hook = _fn, _eaf_group_local->yield.arg = _arg;;)\
		if (_eaf_service_local->branch == 0) {\
			case (n): ;\
			break;\
		} else\
		switch (_eaf_service_local->branch ? 0 : 1)\
			for(;;)\
				/* fall-through */ case -1: if (_eaf_service_local->branch)\
					goto terminate_coroutine;\
				else for (;;)\
					/* fall-through */ case 1: if (_eaf_service_local->branch)\
					goto bail_out_of_coroutine;\
				else /* fall-through */ case 0: { };

#define EAF_SERVICE_CC0_YIELD		(0x01 << 0x00)	/** yield */

/**
* yield hook
* @param id		service id
* @param arg	user defined arg
*/
typedef void(*eaf_yield_hook_fn)(uint32_t id, void* arg);

typedef struct eaf_service_local
{
	uint32_t				id;			/** current service id */
	uint32_t				branch;		/** yield branch */
}eaf_service_local_t;

typedef struct eaf_group_local
{
	uint32_t				cc[1];		/** coroutine control */

	struct
	{
		eaf_yield_hook_fn	hook;		/** hook */
		void*				arg;		/** user arg */
	}yield;
}eaf_group_local_t;

/**
* 获取本地数据
* @return		eaf_service_local_t*
*/
eaf_service_local_t* eaf_service_get_local(eaf_group_local_t** local);

#ifdef __cplusplus
}
#endif
#endif
