/** @file
 * IOCP
 */
#ifndef __EAF_POWERPACK_IOCP_H__
#define __EAF_POWERPACK_IOCP_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-IOCP IOCP
 * @{
 */

/**
 * @brief IOCP Service ID
 */
#define EAF_IOCP_ID					(0x00010000)

/**
 * @ingroup PowerPack-IOCP
 * @defgroup PowerPack-IOCP-Connect Connect
 * @brief Connect to remote
 *
 * The #EAF_IOCP_MSG_CONNECT_REQ create a `handler' and connect to the address
 * specified by #eaf_iocp_connect_req_t.
 *
 * The #EAF_IOCP_MSG_CONNECT_RSP is the result of this operation, which is one
 * of following branches:
 * + Negative value: The operation is failure and the value is #eaf_errno.
 * + Positive value: The operation is success and the value is valid.
 *
 * @{
 */

/**
 * @brief Request ID for connect
 * @see eaf_iocp_connect_req_t
 */
#define EAF_IOCP_MSG_CONNECT_REQ	(EAF_IOCP_ID + 0x0001)
/**
 * @brief Request for connect
 * @see EAF_IOCP_MSG_CONNECT_REQ
 */
typedef struct eaf_iocp_connect_req
{
	int			tcp;			/**< non-zero if TCP, zero if UDP */
	char		addr[64];		/**< Destination addr */
	int			port;			/**< Destination port */
}eaf_iocp_connect_req_t;
/**
 * @brief Response ID for connect
 * @see eaf_iocp_connect_rsp_t
 */
#define EAF_IOCP_MSG_CONNECT_RSP	EAF_IOCP_MSG_CONNECT_REQ
/**
 * @brief Response for connect
 * @see EAF_IOCP_MSG_CONNECT_RSP
 */
typedef struct eaf_iocp_connect_rsp
{
	int			handler;		/**< A positive value if success, a negative value if failure */
}eaf_iocp_connect_rsp_t;

/**
 * @}
 */

/**
 * @ingroup PowerPack-IOCP
 * @defgroup PowerPack-IOCP-Listen Listen
 * @brief Listen local address.
 * @{
 */

#define EAF_IOCP_MSG_LISTEN_REQ		(EAF_IOCP_ID + 0x0002)
typedef struct eaf_iocp_listen_req
{
	int			tcp;			/**< non-zero if TCP, zero if UDP */
	char		addr[64];		/**< Destination addr */
	int			port;			/**< Destination port */
	uint32_t	msgid;			/**< When new connection is established, send request to this msgid */
}eaf_iocp_listen_req_t;
#define EAF_IOCP_MSG_LISTEN_RSP		EAF_IOCP_MSG_LISTEN_REQ
typedef struct eaf_iocp_listen_rsp
{
	int			handler;		/**< A positive value if success, a negative value if failure */
}eaf_iocp_listen_rsp_t;

typedef struct eaf_iocp_newconn_req
{
	int			handler;		/**< Incoming connect handler */
}eaf_iocp_newconn_req_t;
typedef struct eaf_iocp_newconn_rsp
{
	int			ret;			/**< Result */
}eaf_iocp_newconn_rsp_t;

/**
 * @}
 */

/**
 * @ingroup PowerPack-IOCP
 * @defgroup PowerPack-IOCP-Close Close
 * @{
 */

#define EAF_IOCP_MSG_CLOSE_REQ		(EAF_IOCP_ID + 0x0003)
typedef struct eaf_iocp_close_req
{
	int			handler;		/**< Handler */
}eaf_iocp_close_req_t;
#define EAF_IOCP_MSG_CLOSE_RSP		EAF_IOCP_MSG_CLOSE_REQ
typedef struct eaf_iocp_close_rsp
{
	int			ret;			/**< Result */
}eaf_iocp_close_rsp_t;

/**
 * @}
 */

/**
 * @ingroup PowerPack-IOCP
 * @defgroup PowerPack-IOCP-Send Send
 * @{
 */

#define EAF_IOCP_MSG_SEND_REQ		(EAF_IOCP_ID + 0x0004)
typedef struct eaf_iocp_send_req
{
	int			handler;		/**< Handler*/
	size_t		size;			/**< Data size*/
	void*		data;			/**< Data */
}eaf_iocp_send_req_t;
#define EAF_IOCP_MSG_SEND_RSP		EAF_IOCP_MSG_SEND_REQ
typedef struct eaf_iocp_send_rsp
{
	int			ret;			/**< Result */
}eaf_iocp_send_rsp_t;

/**
 * @}
 */

/**
 * @ingroup PowerPack-IOCP
 * @defgroup PowerPack-IOCP-Recv Recv
 * @{
 */

#define EAF_IOCP_MSG_RECV_REQ		(EAF_IOCP_ID + 0x0005)
typedef struct eaf_iocp_recv_req
{
	int			handler;		/**< Handler*/
	size_t		size;			/**< Buffer size*/
	void*		buffer;			/**< Buffer */
}eaf_iocp_recv_req_t;
#define EAF_IOCP_MSG_RECV_RSP		EAF_IOCP_MSG_RECV_REQ
typedef struct eaf_iocp_recv_rsp
{
	int			ret;			/**< Result */
}eaf_iocp_recv_rsp_t;

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
