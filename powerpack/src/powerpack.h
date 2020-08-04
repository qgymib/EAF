#ifndef __EAF_POWERPACK_INTERNAL_H__
#define __EAF_POWERPACK_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/powerpack.h"
#include "uv.h"

/**
 * @brief A Synchronize communication tool
 */
typedef struct pp_sync
{
	struct
	{
		unsigned	have_req : 1;
		unsigned	have_rsp : 1;
	}mask;

	struct
	{
		int			req;
		int			rsp;
	}code;

	struct
	{
		uv_sem_t	sem_req;
		uv_sem_t	sem_rsp;
		uv_mutex_t	objlock;
	}sync;
}pp_sync_t;

/**
 * @brief Initialize #pp_sync_t
 * @return	#eaf_errno
 */
int pp_sync_init(pp_sync_t* handle);

/**
 * @brief Exit #pp_sync_t
 */
void pp_sync_exit(pp_sync_t* handle);

/**
 * @brief Send request and wait for response
 * @param[in] handle	A handler for #pp_sync_t
 * @param[in] req		Request code
 * @return				Response code
 */
int pp_sync_dial(pp_sync_t* handle, int req);

/**
 * @brief Send request and wait for response
 *
 * If already dialed, the response will return directly.
 *
 * @param[in] handle	A handler for #pp_sync_t
 * @param[in] req		Request code
 * @return				Response code
 */
int pp_sync_dial_once(pp_sync_t* handle, int req);

/**
 * @brief Wait for request
 * @param[in] handle	A handler for #pp_sync_t
 * @return				Request code
 */
int pp_sync_wait(pp_sync_t* handle);

/**
 * @brief Send response
 * @param[in] handle	A handler for #pp_sync_t
 * @param[in] rsp		Response code
 */
void pp_sync_answer(pp_sync_t* handle, int rsp);

typedef struct pp_ctx
{
	eaf_thread_t*	working;		/** working thread */

	struct
	{
		unsigned	initialized : 1;
		unsigned	looping : 1;
	}mask;

	struct
	{
		eaf_list_t	table;
		eaf_hook_t	inject;
	}hook;
}pp_ctx_t;

typedef struct uv_ctx
{
	struct
	{
		unsigned	init_failed : 1;
	}mask;

	pp_sync_t		init_point;			/**< Initialize point */
	pp_sync_t		exit_point;			/**< Exit point */

	uv_loop_t		uv_loop;			/**< libuv loop */
	uv_async_t		uv_async_mode;		/**< libuv async notifier */
}uv_ctx_t;

/**
 * @brief Get uv_loop
 * @return		uv_loop_t
 */
uv_loop_t* eaf_uv_get(void);

/**
 * @brief Notify that uv_loop has modified
 */
void eaf_uv_mod(void);

typedef struct pp_init_item
{
	/**
	 * @brief Generic initialize callback
	 * @return	zero if success, non-zero if failure
	 */
	int(*on_init)(void);

	/**
	 * @brief Generic exit callback
	 */
	void(*on_exit)(void);
}pp_init_item_t;

#ifdef __cplusplus
}
#endif
#endif
