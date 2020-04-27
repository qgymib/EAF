/** @file
 * Network
 */
#ifndef __EAF_POWERPACK_NET_H__
#define __EAF_POWERPACK_NET_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/eaf.h"

/**
 * @brief System socket
 */
#if defined(_MSC_VER)
#include <WinSock2.h>
typedef SOCKET eaf_socket_t;
#else
typedef int eaf_socket_t;
#endif

/**
 * @brief Socket events for input/output
 */
typedef enum eaf_socket_event
{
	eaf_socket_event_in			= 0x01 << 0x00,		/**< Socket is available for read operations */
	eaf_socket_event_out		= 0x01 << 0x01,		/**< Socket is available for write operations */

	eaf_socket_event_timeout	= 0x01 << 0x02,		/**< (output only) Operation timeout */
	eaf_socket_event_error		= 0x01 << 0x03,		/**< (output only) Error condition happened on the socket */
}eaf_socket_event_t;

/**
* @brief Wait for socket events
* @see eaf_socket_event
* @param[out] ret		Result as #eaf_socket_event
* @param[in] sock		System socket
* @param[in] evts		Events as #eaf_socket_event
* @param[in] timeout	Timeout in milliseconds
*/
#define eaf_socket_wait(ret, sock, evts, timeout)	\
	do {\
		if (((ret) = eaf_powerpack_net_socket_wait_setup(_eaf_local->id, sock, evts, timeout)) < 0) {\
			break;\
		}\
		eaf_yield_ext(eaf_powerpack_net_socket_wait_commit, NULL);\
		(ret) = (int)(_eaf_local->unsafe[0].v_long);\
	} while (0)

/**
 * @brief (Internal) Setup context for #eaf_socket_wait
 * @note This function should only for internal usage.
 * @see eaf_socket_wait
 * @param[in] id			Service ID
 * @param[in] sock		Socket
 * @param[in] evts		Events
 * @param[in] timeout	Timeout
 * @return				#eaf_errno
 */
int eaf_powerpack_net_socket_wait_setup(_In_ uint32_t id,
	_In_ eaf_socket_t sock, _In_ unsigned evts, _In_ unsigned timeout);

/**
 * @brief (Internal) Perform `eaf_socket_wait`
 * @note This function should only for internal usage.
 * @see eaf_socket_wait
 * @param[in,out] local		Service local storage
 * @param[in,out] arg		NULL
 */
void eaf_powerpack_net_socket_wait_commit(_Inout_ eaf_service_local_t* local,
	_Inout_opt_ void* arg);

#ifdef __cplusplus
}
#endif
#endif
