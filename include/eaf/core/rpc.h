/** @file
 * RPC support for EAF
 */
#ifndef __EAF_CORE_RPC_H__
#define __EAF_CORE_RPC_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/core/message.h"
#include "eaf/utils/annotations.h"

/**
 * @brief Service info
 * @see eaf_rpc_cfg_t::on_service_register
 */
typedef struct eaf_rpc_service_info
{
	uint32_t	service_id;		/**< Service ID */
	uint32_t	capacity;		/**< Message queue capacity */
}eaf_rpc_service_info_t;

/**
 * @brief RPC handler
 */
typedef struct eaf_rpc_cfg
{
	/**
	 * @brief Service registered.
	 * @param[in] info	service info
	 */
	void(*on_service_register)(_In_ const eaf_rpc_service_info_t* info);

	/**
	 * Initialize process must finish.
	 * @return		0 if success, otherwise failed.
	 */
	int(*on_init_done)(void);

	/**
	 * @brief Shutdown RPC.
	 *
	 * After this call finish, you must guarantee no existing/future `eaf_rpc_income` call.
	 */
	void(*on_exit)(void);

	/**
	 * @brief Send request to remote
	 * @param[in] to			Who will receive this request
	 * @param[in,out] req		Request
	 * @return					#eaf_errno
	 */
	int(*output_req)(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* req);

	/**
	 * @brief Send response to remote
	 * @param[in] receipt		Receipt for request
	 * @param[in] to			Who will receive this response
	 * @param[in,out] rsp		Response
	 * @return					#eaf_errno
	 */
	int(*output_rsp)(_In_ int receipt, _In_ uint32_t from, _In_ uint32_t to,
		_Inout_ eaf_msg_t* rsp);
} eaf_rpc_cfg_t;

/**
 * @brief Initialize RPC support
 * @param[in] cfg	RPC configuration. Must be a static variable.
 * @return			#eaf_errno
 */
int eaf_rpc_init(_In_ const eaf_rpc_cfg_t* cfg /*static*/);

/**
 * @brief Handle incoming RPC request
 * @param[in] from		Who send this request
 * @param[in] to		Who will receive this request
 * @param[in,out] req	Incoming request
 * @return				#eaf_errno
 */
int eaf_rpc_input_req(_In_ uint32_t from, _In_ uint32_t to, _Inout_ eaf_msg_t* req);

/**
 * @brief Handle incoming RPC response
 * @param[in] receipt	The receipt for request
 * @param[in] from		Who send this response
 * @param[in] to		Who will receive this response
 * @param[in,out] rsp	Incoming response
 * @return				#eaf_errno
 */
int eaf_rpc_input_rsp(_In_ int receipt, _In_ uint32_t from, _In_ uint32_t to,
	_Inout_ eaf_msg_t* rsp);

#ifdef __cplusplus
}
#endif
#endif
