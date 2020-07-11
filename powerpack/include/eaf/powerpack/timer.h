/** @file
 * Enhance timer
 */
#ifndef __EAF_POWERPACK_TIMER_H__
#define __EAF_POWERPACK_TIMER_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-Timer Timer
 * @{
 */

#include "eaf/eaf.h"

/**
 * @brief Timer Service ID
 */
#define EAF_TIMER_ID				(0x00010000)

/**
 * @ingroup PowerPack-Timer
 * @defgroup PowerPack-Timer-Delay Delay
 * @{
 */

#define EAF_TIMER_MSG_DELAY_REQ		(EAF_TIMER_ID + 0x0001)
typedef struct eaf_timer_delay_req
{
	uint32_t	msec;
}eaf_timer_delay_req_t;
#define EAF_TIMER_MSG_DELAY_RSP		EAF_TIMER_MSG_DELAY_REQ
typedef struct eaf_timer_delay_rsp
{
	int32_t		ret;
}eaf_timer_delay_rsp_t;

/**
 * @brief A helper function for create timer delay request
 * @param[in] msec		Delay timeout
 * @param[in] handle	Response handler
 * @return				Request message with reference count set to 1.
 */
int eaf_timer_delay(uint32_t msec, eaf_msg_handle_fn handle);

/**
 * @}
 */

/**
 * Initialize timer
 * @return	eaf_errno
 */
int eaf_timer_init(void);

/**
 * Cleanup timer
 */
void eaf_timer_exit(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
