#ifndef __TEST_QUICK2_IUNTERNAL_H__
#define __TEST_QUICK_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/powerpack.h"
#include "ctest/ctest.h"
#include "utils/dial.h"

/**
 * @brief Log helper
 * @param[in] fmt	Format
 * @param[in] ...	Arguments
 */
#define LOG_TRACE(fmt, ...)	EAF_LOG_TRACE("test", fmt, ##__VA_ARGS__)

/**
 * @brief A macro to help deploy a new service
 * @param[in] gid		Group ID, 0 <= gid <= 8
 * @param[in] sid		Service ID
 * @param[in] fn_init	Service initialize callback
 * @param[in] fn_exit	Service exit callback
 * @param[in] ...		Message table. If not need, use #QUICK_DEPLOY_NO_MSG
 * @see QUICK_DEPLOY_NO_MSG
 */
#define QUICK_DEPLOY_SERVICE(gid, sid, fn_init, fn_exit, ...)	\
	do {\
		static eaf_message_table_t _msg_table[] = __VA_ARGS__;\
		static eaf_entrypoint_t _entry = {\
			EAF_ARRAY_SIZE(_msg_table), _msg_table, fn_init, fn_exit,\
		};\
		ASSERT_EQ_D32(test_quick2_internal_deploy(gid, sid, &_entry), eaf_errno_success,\
			"%s(%d)", eaf_strerror(_a), _a);\
	} EAF_MSVC_WARNING_GUARD(4127, while (0))

/**
 * @brief A macro to help deploy a existing service
 * @param[in] gid	Group ID, 0 <= gid <= 8
 * @param[in] sid	Service ID
 */
#define QUICK_RESERVE_SERVICE(gid, sid)	\
	do {\
		ASSERT_EQ_D32(test_quick2_internal_reserve(gid, sid), 0);\
	} EAF_MSVC_WARNING_GUARD(4127, while (0))

/**
 * @brief Indicate a service do not have message table
 * @see #QUICK_DEPLOY_SERVICE()
 */
#define QUICK_DEPLOY_NO_MSG	{ { 0, NULL } }

/**
 * @brief Force call #eaf_init().
 *
 * By default Quick will call #eaf_init() after #TEST_FIXTURE_SETUP().
 * If you need to do this earlier, use this macro.
 *
 * @note After this macro is called, future #QUICK_DEPLOY_SERVICE() and
 * #QUICK_RESERVE_SERVICE() is invalid.
 *
 */
#define QUICK_FORCE_INIT_EAF()	\
	do {\
		ASSERT_EQ_D32(test_quick2_internal_force_init_eaf(), eaf_errno_success,\
			"%s(%d)", eaf_strerror(_a), _a);\
	} EAF_MSVC_WARNING_GUARD(4127, while (0))

/**
 * @brief Force call #eaf_load()
 *
 * By default Quick will call #eaf_load() after #TEST_FIXTURE_SETUP().
 * If you need to do this earlier, use this macro.
 */
#define QUICK_FORCE_LOAD_EAF()	\
	do {\
		ASSERT_EQ_D32(test_quick2_internal_force_load_eaf(), eaf_errno_success,\
			"%s(%d)", eaf_strerror(_a), _a);\
	} while (0)

typedef struct test_quick_config
{
	eaf_log_level_t	log_level;
}test_quick_config_t;

/**
 * @brief Quick test configuration
 */
extern test_quick_config_t quick_config;

/**
 * @brief A global hook for ctest.
 */
extern const ctest_hook_t quick_hook;

int test_quick2_internal_deploy(uint32_t gid, uint32_t sid, eaf_entrypoint_t* entry);
int test_quick2_internal_reserve(uint32_t gid, uint32_t sid);
int test_quick2_internal_force_init_eaf(void);
int test_quick2_internal_force_load_eaf(void);

#ifdef __cplusplus
}
#endif
#endif
