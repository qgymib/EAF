#include <stdlib.h>
#include <string.h>
#include "eaf/eaf.h"
#include "quick.h"

void test_quick_default_request(uint32_t from, uint32_t to, eaf_msg_t* req)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to);
	int value = *(int*)eaf_msg_get_data(req, NULL);

	eaf_msg_t* rsp = eaf_msg_create_rsp(req, sizeof(int));
	*(int*)eaf_msg_get_data(rsp, NULL) = ~value;

	eaf_send_rsp(eaf_service_self(), req->from, rsp);
	eaf_msg_dec_ref(rsp);
}

void test_quick_request_template_empty(uint32_t from, uint32_t to, eaf_msg_t* req)
{
	EAF_SUPPRESS_UNUSED_VARIABLE(from, to);

	eaf_msg_t* rsp = eaf_msg_create_rsp(req, 0);

	eaf_send_rsp(to, req->from, rsp);
	eaf_msg_dec_ref(rsp);
}
