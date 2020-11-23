/**
 * @file
 */
#ifndef __EAF_POWERPACK_WATCHDOG_H__
#define __EAF_POWERPACK_WATCHDOG_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-WatchDog WatchDog
 * @{
 */

#include "eaf/eaf.h"
#include "eaf/powerpack/time.h"
#include "eaf/powerpack/define.h"

/**
 * @brief WatchDog Service ID
 */
#define EAF_WATCHDOG_ID					(0x00020000)

/**
 * @ingroup PowerPack-WatchDog
 * @defgroup PowerPack-WatchDog-HeartBeat HeartBeat
 * @{
 */

/**
 * @brief Request ID for watchdog message: heartbeat
 */
#define EAF_WATCHDOG_MSG_HEARTBEAT_REQ	(EAF_WATCHDOG_ID + 0x0003)
/**
 * @brief Request for watchdog message: heartbeat
 */
typedef struct eaf_watchdog_heartbeat_req
{
	/**
	 * Service will be consider as abnormal if response was not handled by watchdog
	 * before `threshold'.
	 * @see eaf_time_getclock()
	 */
	eaf_clock_time_t	threshold;
}eaf_watchdog_heartbeat_req_t;
/**
 * @brief Response ID for watchdog message: heartbeat
 */
#define EAF_WATCHDOG_MSG_HEARTBEAT_RSP	EAF_WATCHDOG_MSG_HEARTBEAT_REQ
/**
 * @brief Response for watchdog message: heartbeat
 */
typedef struct eaf_watchdog_heartbeat_rsp
{
	int32_t				ret;			/**< Result */
}eaf_watchdog_heartbeat_rsp_t;

/**
 * @}
 */

/**
 * @brief WatchDog timeout callback
 * @param[in] sid		Service ID
 * @param[in,out] arg	User defined argument
 */
typedef void(*eaf_watchdog_on_error_fn)(_In_ uint32_t sid, _Inout_opt_ void* arg);

typedef struct eaf_watchdog_watch_list
{
	uint32_t		sid;			/**< Service ID */
	uint32_t		interval;		/**< The time between two heartbeat query in millisecond */
	uint32_t		timeout;		/**< The timeout of heartbeat response in millisecond */
	uint32_t		jitter;			/**< The number of EXTRA continuous timeout for the service allowed.
									     Additionally, if response was not received in `jitter * timeout`,
										 the timeout callback will also be called */
}eaf_watchdog_watch_list_t;

/**
 * @brief Initialize WatchDog
 * @param[in] table		The services need to watch. This value must be globally
 *                      accessable and cannot be freed until watchdog exit.
 * @param[in] size		Table size
 * @param[in] fn		WatchDog timeout callback
 * @param[in,out] arg	User defined argument
 * @return				#eaf_errno
 */
int eaf_watchdog_init(_In_ const eaf_watchdog_watch_list_t* table, _In_ size_t size,
	_In_ eaf_watchdog_on_error_fn fn, _Inout_opt_ void* arg);

/**
 * @brief Exit Watchdog
 */
void eaf_watchdog_exit(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
