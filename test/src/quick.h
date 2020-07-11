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

#define QUICK_SETUP_CFG(pcfg, idx, id, init, exit, p0, p1, p2, p3)	\
	do {\
		test_quick_cfg_t* cfg = pcfg;\
		size_t i = idx;\
		cfg->entry[i].ability.enable = 1;\
		cfg->entry[i].on_init = init;\
		cfg->entry[i].on_exit = exit;\
		cfg->entry[i].msg_map[0] = p0;\
		cfg->entry[i].msg_map[1] = p1;\
		cfg->entry[i].msg_map[2] = p2;\
		cfg->entry[i].msg_map[3] = p3;\
	} EAF_MSVC_PUSH_WARNNING(4127) while (0) EAF_MSVC_POP_WARNNING()

#define QUICK_SETUP_BEFPRELOAD(pcfg, proc, priv)	\
	do {\
		test_quick_cfg_t* cfg = pcfg;\
		cfg->before_load.fn = proc;\
		cfg->before_load.arg = priv;\
	} EAF_MSVC_PUSH_WARNNING(4127) while (0) EAF_MSVC_POP_WARNNING()

typedef struct test_quick_cfg
{
	struct
	{
		struct
		{
			unsigned			enable : 1;
		}ability;
		eaf_msg_handle_fn		msg_map[4];				/**< Each service has 4 messages */
		int						(*on_init)(void);		/**< Initialize callback */
		void					(*on_exit)(void);		/**< Exit callback */
	}entry[4];

	struct
	{
		void					(*fn)(void* arg);		/**< Callback before load */
		void*					arg;					/**< User defined argument */
	}before_load;
}test_quick_cfg_t;

/**
 * @brief A global quick configuration for minimize program size.
 */
extern test_quick_cfg_t g_quick_cfg;

/**
 * @brief Setup EAF
 * @param[in] cfg	Configure
 * @return			#eaf_errno
 */
int test_eaf_quick_setup(const test_quick_cfg_t* cfg);

/**
 * @brief Exit EAF
 */
void test_eaf_quick_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif
