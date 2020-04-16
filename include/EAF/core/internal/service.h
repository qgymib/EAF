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
	eaf_group_local_t* _eaf_glocal = NULL;\
	eaf_service_local_t* _eaf_local = eaf_service_get_local(&_eaf_glocal);\
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

#define EAF_COROUTINE_YIELD(_fn, _arg, n)	\
	for (_eaf_local->branch = (n), _eaf_glocal->cc[0] = EAF_SERVICE_CC0_YIELD,\
		_eaf_glocal->yield.hook = _fn, _eaf_glocal->yield.arg = _arg;;)\
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
				else /* fall-through */ case 0: { };

#define EAF_SERVICE_CC0_YIELD		(0x01 << 0x00)	/** yield */

typedef struct eaf_service_local
{
	uint32_t					id;			/** Current service id. DO NOT modify it. */
	uint32_t					branch;		/** Yield branch. DO NOT modify it */

	/*
	* the field `unsafe` is used to pass value cross yield.
	* use with careful.
	*/
	union
	{
		signed int				v_int;		/** unsafe value: signed int */
		unsigned int			v_uint;		/** unsafe value: unsigned int */
		signed long				v_long;		/** unsafe value: signed long */
		unsigned long			v_ulong;	/** unsafe value: unsigned long */
		signed long long		v_llong;	/** unsafe value: signed long long */
		unsigned long long		v_ullong;	/** unsafe value: unsigned long long */
		float					v_float;	/** unsafe value: float */
		double					v_double;	/** unsafe value: double */
		int32_t					v_int32;	/** unsafe value: int32_t */
		uint32_t				v_uint32;	/** unsafe value: uint32_t */
		int64_t					v_int64;	/** unsafe value: int64_t */
		uint64_t				v_uint64;	/** unsafe value: uint64_t */
		intptr_t				v_intptr;	/** unsafe value: intptr_t */
		uintptr_t				v_uintptr;	/** unsafe value: v_uintptr */
		void*					v_ptr;		/** unsafe value: void* */
	}unsafe;
}eaf_service_local_t;

/**
* yield hook
* @param id		service id
* @param arg	user defined arg
*/
typedef void(*eaf_yield_hook_fn)(eaf_service_local_t* local, void* arg);

typedef struct eaf_group_local
{
	uint32_t					cc[1];		/** coroutine control */

	struct
	{
		eaf_yield_hook_fn		hook;		/** hook */
		void*					arg;		/** user arg */
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
