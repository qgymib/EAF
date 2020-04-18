#ifndef __EAF_POWERPACK_NET_H__
#define __EAF_POWERPACK_NET_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "EAF/core/service.h"

#if defined(_MSC_VER)
#include <WinSock2.h>
typedef SOCKET eaf_socket_t;
#else
typedef int eaf_socket_t;
#endif

typedef enum eaf_socket_event
{
	eaf_socket_event_in			= 0x01 << 0x00,		/** Socket is available for read operations */
	eaf_socket_event_out		= 0x01 << 0x01,		/** Socket is available for write operations */

	eaf_socket_event_timeout	= 0x01 << 0x02,		/** (output only) Operation timeout */
	eaf_socket_event_error		= 0x01 << 0x03,		/** (output only) Error condition happened on the socket */
}eaf_socket_event_t;

/**
* Wait for socket events
* @param ret		return value
* @param sock		socket
* @param evts		events
* @param timeout	timeout in milliseconds
*/
#define eaf_socket_wait(ret, sock, evts, timeout)	\
	do {\
		if (((ret) = eaf_powerpack_net_socket_wait_setup(_eaf_local->id, sock, evts, timeout)) < 0) {\
			break;\
		}\
		eaf_yield_ext(eaf_powerpack_net_socket_wait_commit, NULL);\
		(ret) = _eaf_local->unsafe.v_int;\
	} while (0)

/**
* (Internal) Setup context for `eaf_socket_wait`
* @see eaf_socket_wait
* @param id			service id
* @param sock		socket
* @param evts		events
* @param timeout	timeout
* @return			errno
*/
int eaf_powerpack_net_socket_wait_setup(uint32_t id, eaf_socket_t sock, unsigned evts, unsigned timeout);

/**
* (Internal) Perform `eaf_socket_wait`
* @param local		service local storage
* @param arg		NULL
*/
void eaf_powerpack_net_socket_wait_commit(eaf_service_local_t* local, void* arg);

#ifdef __cplusplus
}
#endif
#endif
