#include "EAF/utils/define.h"
#include "utils/memory.h"
#include "message.h"

static eaf_msg_full_t* _eaf_msg_create(eaf_msg_type_t type, uint32_t id, size_t size)
{
	size_t malloc_size = sizeof(eaf_msg_full_t) + size;
	eaf_msg_full_t* msg = EAF_MALLOC(malloc_size);
	if (msg == NULL)
	{
		return NULL;
	}

	if (eaf_mutex_init(&msg->objlock, eaf_mutex_attr_normal) < 0)
	{
		EAF_FREE(msg);
		return NULL;
	}

	msg->msg.type = type;
	msg->msg.id = id;
	msg->msg.from = -1;
	msg->msg.to = -1;
	msg->cnt.refcnt = 1;
	msg->data.size = size;

	return msg;
}

static void _eaf_msg_destroy(eaf_msg_full_t* msg)
{
	if (msg->msg.type == eaf_msg_type_rsp
		&& msg->msg.info.rsp.orig != NULL)
	{
		eaf_msg_dec_ref(msg->msg.info.rsp.orig);
		msg->msg.info.rsp.orig = NULL;
	}

	eaf_mutex_exit(&msg->objlock);
	EAF_FREE(msg);
}

eaf_msg_t* eaf_msg_create_req(uint32_t msg_id, size_t size, eaf_rsp_handle_fn rsp_fn)
{
	eaf_msg_full_t* msg = _eaf_msg_create(eaf_msg_type_req, msg_id, size);
	if (msg == NULL)
	{
		return NULL;
	}

	msg->info.req.rsp_fn = rsp_fn;

	return EAF_MSG_C2I(msg);
}

eaf_msg_t* eaf_msg_create_rsp(eaf_msg_t* req, size_t size)
{
	eaf_msg_full_t* real_req = EAF_MSG_I2C(req);
	eaf_msg_full_t* msg = _eaf_msg_create(eaf_msg_type_rsp, req->id, size);
	if (msg == NULL)
	{
		return NULL;
	}

	msg->msg.to = real_req->msg.from;
	msg->info.rsp.rsp_fn = real_req->info.req.rsp_fn;

	msg->msg.info.rsp.orig = req;
	eaf_msg_add_ref(req);

	return &msg->msg;
}

eaf_msg_t* eaf_msg_create_evt(uint32_t evt_id, size_t size)
{
	eaf_msg_full_t* msg = _eaf_msg_create(eaf_msg_type_evt, evt_id, size);
	if (msg == NULL)
	{
		return NULL;
	}

	return &msg->msg;
}

void eaf_msg_add_ref(eaf_msg_t* msg)
{
	eaf_msg_full_t* real_req = EAF_MSG_I2C(msg);

	eaf_mutex_enter(&real_req->objlock);
	do 
	{
		real_req->cnt.refcnt++;
	} while (0);
	eaf_mutex_leave(&real_req->objlock);
}

void eaf_msg_dec_ref(eaf_msg_t* msg)
{
	eaf_msg_full_t* real_req = EAF_MSG_I2C(msg);

	int need_destroy;
	eaf_mutex_enter(&real_req->objlock);
	do 
	{
		need_destroy = !(--real_req->cnt.refcnt);
	} while (0);
	eaf_mutex_leave(&real_req->objlock);

	if (!need_destroy)
	{
		return;
	}

	_eaf_msg_destroy(real_req);
}

void* eaf_msg_get_data(eaf_msg_t* msg, size_t* size)
{
	eaf_msg_full_t* real_req = EAF_MSG_I2C(msg);

	if (size != NULL)
	{
		*size = real_req->data.size;
	}
	return real_req->data.data;
}
