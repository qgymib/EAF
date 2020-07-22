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
	eaf_group_local_t* _eaf_gls = NULL;\
	eaf_service_local_t* _eaf_sls = eaf_service_get_local(&_eaf_gls);\
	switch(_eaf_sls->branch)\
		case (unsigned)-1: if (_eaf_sls->branch)\
		{\
			goto terminate_coroutine;\
		terminate_coroutine:\
			_eaf_sls->branch = (unsigned)-1;\
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
	for (_eaf_sls->branch = (n), _eaf_gls->cc[0] = EAF_SERVICE_CC0_YIELD,\
		_eaf_gls->yield.hook = _fn, _eaf_gls->yield.arg = _arg;;)\
		if (_eaf_sls->branch == 0) {\
			case (n): ;\
			break;\
		} else\
		switch (_eaf_sls->branch ? 0 : 1)\
			for(;;)\
				/* fall-through */ case -1: if (_eaf_sls->branch)\
					goto terminate_coroutine;\
				else for (;;)\
					/* fall-through */ case 1: if (_eaf_sls->branch)\
					goto bail_out_of_coroutine;\
				else /* fall-through */ case 0: { };

#define EAF_SERVICE_CC0_YIELD		(0x01 << 0x00)	/**< Control Bit 0: service yield */

/**
 * @brief Service states
 * @code
 * INIT_YIELD
 *    /|\
 *     |        |--------|
 *    \|/      \|/       |
 *   INIT --> IDLE --> BUSY --> YIELD
 *     |        |       /|\       |
 *     |       \|/       |--------|
 *     | ----> EXIT
 * @endcode
 */
typedef enum eaf_service_state
{
	eaf_service_state_init,						/**< Init */
	eaf_service_state_init_yield,				/**< Init but user yield */
	eaf_service_state_idle,						/**< No pending work */
	eaf_service_state_busy,						/**< Busy */
	eaf_service_state_yield,					/**< Wait for resume */
	eaf_service_state_exit,						/**< Exit */
}eaf_service_state_t;

/**
 * @brief Service Local Storage (SLS)
 */
typedef struct eaf_service_local
{
	uint32_t					id;			/**< Current service id. DO NOT modify it. */
	uint32_t					branch;		/**< Yield branch. DO NOT modify it */
	eaf_service_state_t			state;		/**< Service state */

	union
	{
		float					v_f32;		/**< unsafe value: float */
		double					v_f64;		/**< unsafe value: double */
		int32_t					v_d32;		/**< unsafe value: int32_t */
		uint32_t				v_u32;		/**< unsafe value: uint32_t */
		int64_t					v_d64;		/**< unsafe value: int64_t */
		uint64_t				v_u64;		/**< unsafe value: uint64_t */
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
 * @brief Group Local Storage (GLS)
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
 * @return				Service Local Storage
 */
EAF_API eaf_service_local_t* eaf_service_get_local(
	_Outptr_opt_result_maybenull_ eaf_group_local_t** gls);

/**
 * @brief Returns an iterator to the beginning.
 * @return				The begin node of Group Local Storage
 */
EAF_API eaf_group_local_t* eaf_group_begin(void);

/**
 * @brief Get an iterator next to the given one.
 * @param[in] gls	Current Group Local Storage node
 * @return			Next Group Local Storage node
 */
EAF_API eaf_group_local_t* eaf_group_next(eaf_group_local_t* gls);

/**
 * @brief Returns an iterator to the beginning.
 * @param[in] gls	Current Group Local Storage node
 * @return			The begin node of Service Local Storage
 */
EAF_API eaf_service_local_t* eaf_service_begin(eaf_group_local_t* gls);

/**
 * @brief Get an iterator next to the given one.
 * @param[in] gls	Current Group Local Storage node
 * @param[in] sls	Current Service Local Storage node
 * @return			Next Service Local Storage node
 */
EAF_API eaf_service_local_t* eaf_service_next(eaf_group_local_t* gls, eaf_service_local_t* sls);

/**
 * @brief Get message queue size.
 * @param[in] sls	Service Local Storage
 * @return			The size of message queue
 */
EAF_API size_t eaf_message_queue_size(_In_ const eaf_service_local_t* sls);

/**
 * @brief Get message queue capacity.
 * @param[in] sls	Service Local Storage
 * @return			The capacity of message queue
 */
EAF_API size_t eaf_message_queue_capacity(_In_ const eaf_service_local_t* sls);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
