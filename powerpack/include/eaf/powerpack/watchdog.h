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
#include "eaf/powerpack/define.h"

/**
 * @brief WatchDog Service ID
 */
#define EAF_WATCHDOG_ID					(0x00020000)

/**
 * @ingroup PowerPack-WatchDog
 * @defgroup PowerPack-WatchDog-Register Register
 * @{
 */

/**
 * @brief Request ID for watchdog message: register
 */
#define EAF_WATCHDOG_MSG_REGISTER_REQ	(EAF_WATCHDOG_ID + 0x0001)
/**
 * @brief Request for watchdog message: register
 */
typedef struct eaf_watchdog_register_req
{
	uint32_t		id;			/**< Unique ID, typically your Service ID */

	/**
	* Timeout in milliseconds.
	* + If non-zero, user must send #EAF_WATCHDOG_MSG_HEARTBEAT_REQ to watchdog
	*   within `timeout', otherwise will trigger reboot.
	*/
	uint32_t		timeout;
}eaf_watchdog_register_req_t;
/**
 * @brief Response ID for watchdog message: register
 */
#define EAF_WATCHDOG_MSG_REGISTER_RSP	EAF_WATCHDOG_MSG_REGISTER_REQ
/**
 * @brief Response for watchdog message: register
 */
typedef struct eaf_watchdog_register_rsp
{
	int32_t			ret;		/**< Result */
}eaf_watchdog_register_rsp_t;

/**
 * @}
 */

/**
 * @ingroup PowerPack-WatchDog
 * @defgroup PowerPack-WatchDog-UnRegister UnRegister
 * @{
 */

/**
 * @brief Request ID for watchdog message: unregister
 */
#define EAF_WATCHDOG_MSG_UNREGISTER_REQ		(EAF_WATCHDOG_ID + 0x0002)
/**
 * @brief Request for watchdog message: unregister
 */
typedef struct eaf_watchdog_unregister_req
{
	uint32_t		id;			/**< Unique ID */
}eaf_watchdog_unregister_req_t;
/**
 * @brief Response ID for watchdog message: unregister
 */
#define EAF_WATCHDOG_MSG_UNREGISTER_RSP		EAF_WATCHDOG_MSG_UNREGISTER_REQ
/**
 * @brief Response for watchdog message: unregister
 */
typedef struct eaf_watchdog_unregister_rsp
{
	int32_t			ret;		/**< Result */
}eaf_watchdog_unregister_rsp_t;

/**
 * @}
 */

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
	uint32_t		id;			/**< Unique ID */
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
	int32_t			ret;		/**< Result */
}eaf_watchdog_heartbeat_rsp_t;

/**
 * @}
 */

/**
 * @brief WatchDog timeout callback
 * @param[in] id		Unique ID
 * @param[in,out] arg	User defined argument
 */
typedef void(*eaf_watchdog_on_error_fn)(_In_ uint32_t id, _Inout_opt_ void* arg);

/**
 * @brief Initialize WatchDog
 * @param[in] fn		WatchDog timeout callback
 * @param[in,out] arg	User defined argument
 * @return				#eaf_errno
 */
int eaf_watchdog_init(_In_ eaf_watchdog_on_error_fn fn, _Inout_opt_ void* arg);

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
