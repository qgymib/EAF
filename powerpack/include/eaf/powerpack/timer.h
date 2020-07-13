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
 *
 * @note You can use #EAF_TIMER_DELAY helper macro to quickly use this message
 * @{
 */

/**
 * @brief Request ID for timer message: delay
 */
#define EAF_TIMER_MSG_DELAY_REQ		(EAF_TIMER_ID + 0x0001)
/**
 * @brief Request for timer message: delay
 */
typedef struct eaf_timer_delay_req
{
	uint32_t	msec;	/**< Timeout in milliseconds */
}eaf_timer_delay_req_t;
/**
 * @brief Response ID for timer message: delay
 */
#define EAF_TIMER_MSG_DELAY_RSP		EAF_TIMER_MSG_DELAY_REQ
 /**
  * @brief Response for timer message: delay
  */
typedef struct eaf_timer_delay_rsp
{
	int32_t		ret;	/**< Result */
}eaf_timer_delay_rsp_t;

/**
 * @brief A helper macro to quickly send delay message
 * @param[out] ret		An integer to store result
 * @param[in] from		Who send this message
 * @param[in] rsp_fn	The function for handle response
 * @param[in] timeout	Timeout in milliseconds
 */
#define EAF_TIMER_DELAY(ret, from, rsp_fn, timeout)	\
	do {\
		eaf_msg_t* req = eaf_msg_create_req(EAF_TIMER_MSG_DELAY_REQ,\
			sizeof(eaf_timer_delay_req_t), rsp_fn);\
		if (req == NULL) {\
			ret = eaf_errno_memory;\
		}\
		((eaf_timer_delay_req_t*)eaf_msg_get_data(req, NULL))->msec = timeout;\
		ret = eaf_send_req(from, EAF_TIMER_ID, req);\
		eaf_msg_dec_ref(req);\
	} EAF_MSVC_WARNING_GUARD(4127, while (0))

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
