/** @file
* Internal implement for service.h
*/
#ifndef __EAF_FILBER_INTERNAL_SERVICE_H__
#define __EAF_FILBER_INTERNAL_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup EAF-Service
 * @defgroup EAF-Service-Internal Internal
 * @{
 */

#include <stddef.h>
#include "eaf/utils/define.h"

/**
 * @def EAF_COROUTINE_YIELD_TOKEN
 * @brief switch-case label
 */
#if defined(_MSC_VER)
#	define EAF_COROUTINE_YIELD_TOKEN	__COUNTER__ + 1
#else
#	define EAF_COROUTINE_YIELD_TOKEN	__LINE__
#endif

/**
 * @brief Implementation for eaf_reenter.
 * It use duff's device to generate a switch-based coroutine.
 */
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

/**
 * @brief Implementation for eaf_yield.
 * It use duff's device to generate a switch-based coroutine.
 * @param[in] _fn	A hook function which will be called after service yield
 * @param[in] _arg	A user defined argument which will passed to _fn
 * @param[in] n		switch-case label
 */
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

#define EAF_SERVICE_CC0_YIELD		(0x01 << 0x00)	/**< Control Bit 0: service yield */

/**
 * @brief Service local storage
 */
typedef struct eaf_service_local
{
	uint32_t					id;			/**< Current service id. DO NOT modify it. */
	uint32_t					branch;		/**< Yield branch. DO NOT modify it */

	union
	{
		long					v_long;		/**< unsafe value: signed long */
		unsigned long			v_ulong;	/**< unsafe value: unsigned long */
		long long				v_llong;	/**< unsafe value: signed long long */
		unsigned long long		v_ullong;	/**< unsafe value: unsigned long long */
		float					v_float;	/**< unsafe value: float */
		double					v_double;	/**< unsafe value: double */
		int64_t					v_int64;	/**< unsafe value: int64_t */
		uint64_t				v_uint64;	/**< unsafe value: uint64_t */
		uintptr_t				v_uintptr;	/**< unsafe value: v_uintptr */
		void*					v_ptr;		/**< unsafe value: void* */
		struct
		{
			uint32_t			w1;			/**< unsafe value: w1 for ww */
			uint32_t			w2;			/**< unsafe value: w2 for ww */
		}ww;								/**< unsafe value: double DWORD */
	}unsafe[1];								/**< The field `unsafe` is used to pass value cross yield. use with careful. */
}eaf_service_local_t;

/**
 * @brief yield hook
 * @param[in,out] local	Service local storage
 * @param[in,out] arg	User defined argument
 */
typedef void(*eaf_yield_hook_fn)(_Inout_ eaf_service_local_t* local, _Inout_ void* arg);

/**
 * @brief Group local storage
 */
typedef struct eaf_group_local
{
	uint32_t					cc[1];		/**< coroutine control */

	struct
	{
		eaf_yield_hook_fn		hook;		/**< hook */
		void*					arg;		/**< user arg */
	}yield;									/**< one time yield hook */
}eaf_group_local_t;

/**
 * @private
 * @brief Get service local storage and group local storage.
 * @param[out] local	A pointer to store group local storage
 * @return				Service local storage
 */
EAF_API eaf_service_local_t* eaf_service_get_local(
	_Outptr_opt_result_maybenull_ eaf_group_local_t** local);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
