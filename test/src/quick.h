#ifndef __TEST_QUICK_INTERNAL_H__
#define __TEST_QUICK_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/eaf.h"
#include "eaf/powerpack/define.h"

#define TEST_QUICK_S0		0xE0000000
#define TEST_QUICK_S0_REQ1	(TEST_QUICK_S0 + 0x0001)
#define TEST_QUICK_S0_REQ2	(TEST_QUICK_S0 + 0x0002)
#define TEST_QUICK_S0_REQ3	(TEST_QUICK_S0 + 0x0003)
#define TEST_QUICK_S0_REQ4	(TEST_QUICK_S0 + 0x0004)

#define TEST_QUICK_S1		0xE0010000
#define TEST_QUICK_S1_REQ1	(TEST_QUICK_S1 + 0x0001)
#define TEST_QUICK_S1_REQ2	(TEST_QUICK_S1 + 0x0002)
#define TEST_QUICK_S1_REQ3	(TEST_QUICK_S1 + 0x0003)
#define TEST_QUICK_S1_REQ4	(TEST_QUICK_S1 + 0x0004)

#define TEST_QUICK_S2		0xE0020000
#define TEST_QUICK_S2_REQ1	(TEST_QUICK_S2 + 0x0001)
#define TEST_QUICK_S2_REQ2	(TEST_QUICK_S2 + 0x0002)
#define TEST_QUICK_S2_REQ3	(TEST_QUICK_S2 + 0x0003)
#define TEST_QUICK_S2_REQ4	(TEST_QUICK_S2 + 0x0004)

#define TEST_QUICK_S3		0xE0030000
#define TEST_QUICK_S3_REQ1	(TEST_QUICK_S3 + 0x0001)
#define TEST_QUICK_S3_REQ2	(TEST_QUICK_S3 + 0x0002)
#define TEST_QUICK_S3_REQ3	(TEST_QUICK_S3 + 0x0003)
#define TEST_QUICK_S3_REQ4	(TEST_QUICK_S3 + 0x0004)

void test_quick_default_request(uint32_t from, uint32_t to, eaf_msg_t* req);
void test_quick_request_template_empty(uint32_t from, uint32_t to, eaf_msg_t* req);

#ifdef __cplusplus
}
#endif
#endif
