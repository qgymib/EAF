/**
 * @file
 */
#ifndef __EAF_POWERPACK_MONITOR_H__
#define __EAF_POWERPACK_MONITOR_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-Monitor Monitor
 * @{
 */

#include "eaf/eaf.h"

/**
 * @brief Monitor Service ID
 */
#define EAF_MONITOR_ID							(0x00040000)

/**
 * @ingroup PowerPack-Monitor
 * @defgroup PowerPack-Monitor-Print Print
 * @{
 */

/**
 * @brief Stringify type
 * @see eaf_monitor_stringify_req_t
 */
typedef enum eaf_monitor_stringify_type
{
	eaf_monitor_stringify_type_normal,			/**< human readable string */
	eaf_monitor_stringify_type_json,			/**< json */
}eaf_monitor_stringify_type_t;

/**
 * @brief Request ID for monitor message: stringify
 */
#define EAF_MINITOR_MSG_STRINGIFY_REQ			(EAF_MONITOR_ID + 0x0001)
/**
 * @brief Request ID for monitor message: stringify
 */
typedef struct eaf_monitor_stringify_req
{
	eaf_monitor_stringify_type_t	type;		/**< String type */
}eaf_monitor_stringify_req_t;
/**
 * @brief Response ID for monitor message: stringify
 */
#define EAF_MINITOR_MSG_STRINGIFY_RSP			EAF_MINITOR_MSG_STRINGIFY_REQ
/**
 * @brief Response for monitor message: stringify
 */
typedef struct eaf_monitor_stringify_rsp
{
	size_t							size;		/**< String length (not include NULL terminator) */
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4200)
#endif
	char							data[];		/**< String data */
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
}eaf_monitor_stringify_rsp_t;

/**
 * @}
 */

/**
 * @brief Initialize monitor
 * @param[in] sec	Refresh time interval
 * @return		#eaf_errno
 */
int eaf_monitor_init(unsigned sec);

/**
 * @brief Exit monitor
 */
void eaf_monitor_exit(void);

/**
 * @brief Reset all necessary counters.
 */
void eaf_monitor_flush(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
