#include "eaf/eaf.h"
#include "etest/etest.h"

TEST(eaf_msg, receipt)
{
	eaf_msg_t* msg = eaf_msg_create_req(0, 0, NULL);
	ASSERT_NE_PTR(msg, NULL);

	eaf_msg_set_receipt(msg, eaf_errno_invalid);
	ASSERT_EQ_D32(eaf_msg_get_receipt(msg), eaf_errno_invalid);

	eaf_msg_dec_ref(msg);
}
