#include "eaf/utils/define.h"
#include "utils/memory.h"
#include "message.h"

static eaf_msg_full_t* _eaf_msg_create(uint32_t id, size_t size)
{
	size_t malloc_size = sizeof(eaf_msg_full_t) + size;
	eaf_msg_full_t* msg = EAF_MALLOC(malloc_size);
	if (msg == NULL)
	{
		return NULL;
	}

	if (eaf_compat_lock_init(&msg->objlock, eaf_lock_attr_normal) < 0)
	{
		EAF_FREE(msg);
		return NULL;
	}

	msg->msg.id = id;
	msg->msg.from = (uint32_t)-1;
	msg->cnt.refcnt = 1;
	msg->data.size = size;

	return msg;
}

static void _eaf_msg_destroy(eaf_msg_full_t* msg)
{
	eaf_compat_lock_exit(&msg->objlock);
	EAF_FREE(msg);
}

eaf_msg_t* eaf_msg_create_req(_In_ uint32_t msg_id, _In_ size_t size, _In_ eaf_rsp_handle_fn rsp_fn)
{
	eaf_msg_full_t* msg = _eaf_msg_create(msg_id, size);
	if (msg == NULL)
	{
		return NULL;
	}

	msg->msg.info.dynamics.encs = EAF_MSG_ENCS_REQ;
	msg->msg.info.constant.uuid = (uintptr_t)msg;
	msg->msg.info.constant.orig = (uintptr_t)rsp_fn;

	return EAF_MSG_C2I(msg);
}

eaf_msg_t* eaf_msg_create_rsp(_In_ eaf_msg_t* req, _In_ size_t size)
{
	eaf_msg_full_t* rsp = _eaf_msg_create(req->id, size);
	if (rsp == NULL)
	{
		return NULL;
	}

	rsp->msg.info.dynamics.encs = 0;
	rsp->msg.info.constant = req->info.constant;

	return EAF_MSG_C2I(rsp);
}

void eaf_msg_add_ref(_Inout_ eaf_msg_t* msg)
{
	eaf_msg_full_t* real_req = EAF_MSG_I2C(msg);

	eaf_compat_lock_enter(&real_req->objlock);
	{
		real_req->cnt.refcnt++;
	}
	eaf_compat_lock_leave(&real_req->objlock);
}

void eaf_msg_dec_ref(_Inout_ eaf_msg_t* msg)
{
	eaf_msg_full_t* real_req = EAF_MSG_I2C(msg);

	int need_destroy;
	eaf_compat_lock_enter(&real_req->objlock);
	{
		need_destroy = !(--real_req->cnt.refcnt);
	}
	eaf_compat_lock_leave(&real_req->objlock);

	if (!need_destroy)
	{
		return;
	}

	_eaf_msg_destroy(real_req);
}

void* eaf_msg_get_data(_In_ eaf_msg_t* msg, _Out_opt_ size_t* size)
{
	eaf_msg_full_t* real_req = EAF_MSG_I2C(msg);

	if (size != NULL)
	{
		*size = real_req->data.size;
	}
	return real_req->data.size != 0 ? real_req->data.data : NULL;
}

eaf_msg_type_t eaf_msg_get_type(_In_ const eaf_msg_t* msg)
{
	return EAF_MSG_IS_REQ(msg) ? eaf_msg_type_req : eaf_msg_type_rsp;
}

void eaf_msg_set_rsp_fn(_Inout_ eaf_msg_t* msg, _In_ eaf_rsp_handle_fn fn)
{
	msg->info.constant.orig = (uint64_t)(uintptr_t)fn;
}

eaf_rsp_handle_fn eaf_msg_get_rsp_fn(_In_ const eaf_msg_t* msg)
{
	return (eaf_rsp_handle_fn)(uintptr_t)msg->info.constant.orig;
}
