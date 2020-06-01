/**
 * @file
 * CTest header file.
 */
#ifndef __CTEST_CTEST_H__
#define __CTEST_CTEST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

/************************************************************************/
/* ctest                                                                */
/************************************************************************/

/**
 * @defgroup CTest
 * @{
 */

/**
 * @defgroup SetupAndTeardown Setup And Teardown
 * @{
 */

/**
 * @brief Setup test suit
 * @note `fixture_name' must be globally unique
 * @param [in] fixture_name		The name of fixture
 */
#define TEST_CLASS_SETUP(fixture_name)	\
	void TEST_CLASS_SETUP_##fixture_name(void)

/**
 * @brief TearDown test suit
 * @note `fixture_name' must be globally unique
 * @param [in] fixture_name		The name of fixture
 */
#define TEST_CLASS_TEAREDOWN(fixture_name)	\
	void TEST_CLASS_TEARDOWN_##fixture_name(void)

/**
 * Group: SetupAndTeardown
 * @}
 */

/**
 * @defgroup DefineTest Define Test
 * @{
 */

/**
 * @brief Get parameterized data
 * @return	The data you defined
 */
#define TEST_GET_PARAM()	\
	(_test_parameterized_data[ctest_internal_parameterized_index()])

/**
 * @brief Define parameterized data for fixture
 * @param [in] fixture_name		Which fixture you want to parameterized
 * @param [in] TYPE				Data type
 * @param [in] ...				Data values
 */
#define TEST_PARAMETERIZED_DEFINE(fixture_name, TYPE, ...)	\
	typedef TYPE _parameterized_type_##fixture_name;\
	TYPE _parameterized_data_##fixture_name[] = { __VA_ARGS__ }

/**
 * @brief Parameterized Test
 * @param [in] fixture_name		The name of fixture
 * @param [in] case_name		The name of test case
 */
#define TEST_P(fixture_name, case_name)	\
	void TEST_##fixture_name##_##case_name(_parameterized_type_##fixture_name*);\
	TEST_INITIALIZER(TEST_INIT_##fixture_name##_##case_name) {\
		static ctest_case_t _case_##fixture_name##_##case_name = {\
			{ { NULL, NULL }, { NULL, NULL, NULL } }, /* .node */\
			{ ctest_case_type_parameterized, 0, #fixture_name, #case_name }, /* .info */\
			{\
				TEST_CLASS_SETUP_##fixture_name,\
				TEST_CLASS_TEARDOWN_##fixture_name,\
				(uintptr_t)TEST_##fixture_name##_##case_name,\
				sizeof(_parameterized_data_##fixture_name) / sizeof(_parameterized_data_##fixture_name[0]),\
				_parameterized_data_##fixture_name,\
			},\
		};\
		ctest_register_case(&_case_##fixture_name##_##case_name);\
	}\
	void TEST_##fixture_name##_##case_name(\
		_parameterized_type_##fixture_name* _test_parameterized_data)

/**
 * @brief Test Fixture
 * @param [in] fixture_name		The name of fixture
 * @param [in] case_name		The name of test case
 */
#define TEST_F(fixture_name, case_name)	\
	void TEST_##fixture_name##_##case_name(void);\
	TEST_INITIALIZER(TEST_INIT_##fixture_name##_##case_name) {\
		static ctest_case_t _case_##fixture_name##_##case_name = {\
			{ { NULL, NULL }, { NULL, NULL, NULL } }, /* .node */\
			{ ctest_case_type_fixture, 0, #fixture_name, #case_name }, /* .info */\
			{\
				TEST_CLASS_SETUP_##fixture_name,\
				TEST_CLASS_TEARDOWN_##fixture_name,\
				(uintptr_t)TEST_##fixture_name##_##case_name,\
				0, NULL\
			},\
		};\
		ctest_register_case(&_case_##fixture_name##_##case_name);\
	}\
	void TEST_##fixture_name##_##case_name(void)

/**
 * @brief Simple Test
 * @param [in] suit_name		suit name
 * @param [in] case_name		case name
 */
#define TEST(suit_name, case_name)	\
	void TEST_##suit_name##_##case_name(void);\
	TEST_INITIALIZER(TEST_INIT_##case_name) {\
		static ctest_case_t _case_##suit_name##_##case_name = {\
			{ { NULL, NULL }, { NULL, NULL, NULL } }, /* .node */\
			{ ctest_case_type_suit, 0, #suit_name, #case_name }, /* .info */\
			{\
				NULL, NULL, (uintptr_t)TEST_##suit_name##_##case_name, 0, NULL\
			},\
		};\
		ctest_register_case(&_case_##suit_name##_##case_name);\
	}\
	void TEST_##suit_name##_##case_name(void)

/**
 * Group: DefineTest
 * @}
 */

/**
 * @defgroup Assertion
 * @{
 */

/**
 * @brief Prints an error message to standard error and terminates the program
 *   by calling abort(3) if expression is false.
 * @note `NDEBUG' has no affect on this macro.
 * @param [in] x		expression
 */
#define ASSERT(x)	\
	((void)((x) || (ctest_unwrap_assert_fail(#x, __FILE__, __LINE__, __FUNCTION__),0)))

/**
 * @def ASSERT_EQ_D32
 * @brief Assert `a' == `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_D32
 * @brief Assert `a' != `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_D32
 * @brief Assert `a' < `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_D32
 * @brief Assert `a' <= `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_D32
 * @brief Assert `a' > `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_D32
 * @brief Assert `a' >= `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_U32
 * @brief Assert `a' == `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_U32
 * @brief Assert `a' != `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_U32
 * @brief Assert `a' < `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_U32
 * @brief Assert `a' <= `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_U32
 * @brief Assert `a' > `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_U32
 * @brief Assert `a' >= `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_X32
 * @brief Assert `a' == `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 8 characters string with prefix `0x' (eg. 0x00000001 or 0xffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_X32
 * @brief Assert `a' != `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 8 characters string with prefix `0x' (eg. 0x00000001 or 0xffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_X32
 * @brief Assert `a' < `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 8 characters string with prefix `0x' (eg. 0x00000001 or 0xffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_X32
 * @brief Assert `a' <= `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 8 characters string with prefix `0x' (eg. 0x00000001 or 0xffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_X32
 * @brief Assert `a' > `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 8 characters string with prefix `0x' (eg. 0x00000001 or 0xffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_X32
 * @brief Assert `a' >= `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 8 characters string with prefix `0x' (eg. 0x00000001 or 0xffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_D64
 * @brief Assert `a' == `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_D64
 * @brief Assert `a' != `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_D64
 * @brief Assert `a' < `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_D64
 * @brief Assert `a' <= `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_D64
 * @brief Assert `a' > `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_D64
 * @brief Assert `a' >= `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_U64
 * @brief Assert `a' == `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_U64
 * @brief Assert `a' != `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_U64
 * @brief Assert `a' < `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_U64
 * @brief Assert `a' <= `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_U64
 * @brief Assert `a' > `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_U64
 * @brief Assert `a' >= `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_X64
 * @brief Assert `a' == `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 16 characters string with prefix `0x' (eg. 0x0000000000000001 or 0xffffffffffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_X64
 * @brief Assert `a' != `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 16 characters string with prefix `0x' (eg. 0x0000000000000001 or 0xffffffffffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_X64
 * @brief Assert `a' < `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 16 characters string with prefix `0x' (eg. 0x0000000000000001 or 0xffffffffffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_X64
 * @brief Assert `a' <= `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 16 characters string with prefix `0x' (eg. 0x0000000000000001 or 0xffffffffffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_X64
 * @brief Assert `a' > `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 16 characters string with prefix `0x' (eg. 0x0000000000000001 or 0xffffffffffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_X64
 * @brief Assert `a' >= `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @note This macro output a fixed 16 characters string with prefix `0x' (eg. 0x0000000000000001 or 0xffffffffffffffff) if compare failure.
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_SIZE
 * @brief Assert `a' == `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_SIZE
 * @brief Assert `a' != `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_SIZE
 * @brief Assert `a' < `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_SIZE
 * @brief Assert `a' <= `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_SIZE
 * @brief Assert `a' > `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_SIZE
 * @brief Assert `a' >= `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%"TEST_PRIsize, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%"TEST_PRIsize, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%"TEST_PRIsize, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%"TEST_PRIsize, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%"TEST_PRIsize, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%"TEST_PRIsize, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_PTR
 * @brief Assert `a' == `b'. `a' and `b' must be pointer type.
 *
 * If `a' != `b' does not match, this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_PTR
 * @brief Assert `a' != `b'. `a' and `b' must be pointer type.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_PTR
 * @brief Assert `a' < `b'. `a' and `b' must be pointer type.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_PTR
 * @brief Assert `a' <= `b'. `a' and `b' must be pointer type.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_PTR
 * @brief Assert `a' > `b'. `a' and `b' must be pointer type.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_PTR
 * @brief Assert `a' >= `b'. `a' and `b' must be pointer type.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_INT
 * @brief Assert `a' == `b'. `a' and `b' must has type `int'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_INT
 * @brief Assert `a' != `b'. `a' and `b' must has type `int'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_INT
 * @brief Assert `a' < `b'. `a' and `b' must has type `int'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_INT
 * @brief Assert `a' <= `b'. `a' and `b' must has type `int'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_INT
 * @brief Assert `a' > `b'. `a' and `b' must has type `int'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_INT
 * @brief Assert `a' >= `b'. `a' and `b' must has type `int'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_UINT
 * @brief Assert `a' == `b'. `a' and `b' must has type `unsigned int'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_UINT
 * @brief Assert `a' != `b'. `a' and `b' must has type `unsigned int'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_UINT
 * @brief Assert `a' < `b'. `a' and `b' must has type `unsigned int'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_UINT
 * @brief Assert `a' <= `b'. `a' and `b' must has type `unsigned int'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_UINT
 * @brief Assert `a' > `b'. `a' and `b' must has type `unsigned int'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_UINT
 * @brief Assert `a' >= `b'. `a' and `b' must has type `unsigned int'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_LONG
 * @brief Assert `a' == `b'. `a' and `b' must has type `long'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_LONG
 * @brief Assert `a' != `b'. `a' and `b' must has type `long'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_LONG
 * @brief Assert `a' < `b'. `a' and `b' must has type `long'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_LONG
 * @brief Assert `a' <= `b'. `a' and `b' must has type `long'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_LONG
 * @brief Assert `a' > `b'. `a' and `b' must has type `long'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_LONG
 * @brief Assert `a' >= `b'. `a' and `b' must has type `long'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_ULONG
 * @brief Assert `a' == `b'. `a' and `b' must has type `unsigned long'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_ULONG
 * @brief Assert `a' != `b'. `a' and `b' must has type `unsigned long'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_ULONG
 * @brief Assert `a' < `b'. `a' and `b' must has type `unsigned long'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_ULONG
 * @brief Assert `a' <= `b'. `a' and `b' must has type `unsigned long'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_ULONG
 * @brief Assert `a' > `b'. `a' and `b' must has type `unsigned long'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_ULONG
 * @brief Assert `a' >= `b'. `a' and `b' must has type `unsigned long'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_FLOAT
 * @brief Assert `a' == `b'. `a' and `b' must has type `float'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_FLOAT
 * @brief Assert `a' != `b'. `a' and `b' must has type `float'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LT_FLOAT
 * @brief Assert `a' < `b'. `a' and `b' must has type `float'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_FLOAT
 * @brief Assert `a' <= `b'. `a' and `b' must has type `float'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_FLOAT
 * @brief Assert `a' > `b'. `a' and `b' must has type `float'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_FLOAT
 * @brief Assert `a' >= `b'. `a' and `b' must has type `float'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", ==, ctest_internal_assert_helper_float_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", !=, !ctest_internal_assert_helper_float_eq, a, b, ##__VA_ARGS__)
#define ASSERT_LT_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", <,  !ctest_internal_assert_helper_float_ge, a, b, ##__VA_ARGS__)
#define ASSERT_LE_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", <=, ctest_internal_assert_helper_float_le,  a, b, ##__VA_ARGS__)
#define ASSERT_GT_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", >,  !ctest_internal_assert_helper_float_le, a, b, ##__VA_ARGS__)
#define ASSERT_GE_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", >=, ctest_internal_assert_helper_float_ge,  a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_DOUBLE
 * @brief Assert `a' == `b'. `a' and `b' must has type `double'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_NE_DOUBLE
 * @brief Assert `a' != `b'. `a' and `b' must has type `double'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a		Value a
 * @param [in] b		Value b
 * @param [in] ...		User defined error message
 */
/**
 * @def ASSERT_LT_DOUBLE
 * @brief Assert `a' < `b'. `a' and `b' must has type `double'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_LE_DOUBLE
 * @brief Assert `a' <= `b'. `a' and `b' must has type `double'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GT_DOUBLE
 * @brief Assert `a' > `b'. `a' and `b' must has type `double'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
/**
 * @def ASSERT_GE_DOUBLE
 * @brief Assert `a' >= `b'. `a' and `b' must has type `double'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a	Value a
 * @param [in] b	Value b
 * @param [in] ...	User defined error message
 */
#define ASSERT_EQ_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", ==, ctest_internal_assert_helper_double_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", !=, !ctest_internal_assert_helper_double_eq, a, b, ##__VA_ARGS__)
#define ASSERT_LT_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", <,  !ctest_internal_assert_helper_double_ge, a, b, ##__VA_ARGS__)
#define ASSERT_LE_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", <=, ctest_internal_assert_helper_double_le,  a, b, ##__VA_ARGS__)
#define ASSERT_GT_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", >,  !ctest_internal_assert_helper_double_le, a, b, ##__VA_ARGS__)
#define ASSERT_GE_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", >=, ctest_internal_assert_helper_double_ge,  a, b, ##__VA_ARGS__)

/**
 * @def ASSERT_EQ_STR
 * @brief Assert `a' == `b'. `a' and `b' must has type `const char*'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a		Value a
 * @param [in] b		Value b
 * @param [in] ...		User defined error message
 */
/**
 * @def ASSERT_NE_STR
 * @brief Assert `a' != `b'. `a' and `b' must has type `const char*'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a		Value a
 * @param [in] b		Value b
 * @param [in] ...		User defined error message
 */
#define ASSERT_EQ_STR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const char*, "%s", ==, ctest_internal_assert_helper_str_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_STR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const char*, "%s", !=, !ctest_internal_assert_helper_str_eq, a, b, ##__VA_ARGS__)

/**
 * Group: Assertion
 * @}
 */

/**
 * @brief Run all test cases
 * @param [in] argc		The number of arguments
 * @param [in] argv		The argument list
 * @return				The number of failure test
 */
int ctest_run_tests(int argc, char* argv[]);

/**
 * @brief Get current running suit name
 * @return				The suit name
 */
const char* ctest_get_current_suit_name(void);

/**
 * @brief Get current running case name
 * @return				The case name
 */
const char* ctest_get_current_case_name(void);

/**
 * @brief Skip current test case
 * @note This function only has affect in setup stage
 * @see TEST_CLASS_SETUP
 */
void ctest_skip_test(void);

/**
 * Group: CTest
 * @}
 */

/************************************************************************/
/* log                                                                  */
/************************************************************************/

/**
 * @addtogroup Log
 * @{
 */

/**
 * @brief Sample Log function
 * @param [in] fmt		Print format
 * @param [in] ...		Print arguments
 */
#define TEST_LOG(fmt, ...)	\
	TEST_EXPAND(printf("[%s:%d %s] " fmt "\n", ctest_pretty_file(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__))

/**
 * @}
 */

/************************************************************************/
/* time                                                                 */
/************************************************************************/

/**
 * @addtogroup Timestamp
 * @{
 */

/**
 * @brief The timestamp
 */
typedef struct ctest_timestamp
{
	uint64_t	sec;		/**< seconds */
	uint64_t	usec;		/**< microseconds */
}ctest_timestamp_t;

/**
 * @brief Monotonic time since some unspecified starting point
 * @param [in] ts		etest_timestamp_t*
 * @return				0 if success, otherwise failure
 */
int ctest_timestamp_get(ctest_timestamp_t* ts);

/**
 * @brief Compare timestamp
 * @param [in] t1		timestamp t1
 * @param [in] t2		timestamp t2
 * @param [in] dif		diff
 * @return				-1 if t1 < t2; 1 if t1 > t2; 0 if t1 == t2
 */
int ctest_timestamp_dif(const ctest_timestamp_t* t1, const ctest_timestamp_t* t2, ctest_timestamp_t* dif);

/**
 * @}
 */

/************************************************************************/
/* inline hook                                                          */
/************************************************************************/

/**
 * @addtogroup Inline hook
 * @{
 */

/**
 * @brief The patch handler
 */
typedef struct ctest_stub ctest_stub_t;

/**
 * @brief Do inline hook
 * @param [in] fn_orig		The origianl function
 * @param [in] fn_stub		The hook function
 * @return					The patch handler
 */
ctest_stub_t* ctest_patch(uintptr_t fn_orig, uintptr_t fn_stub);

/**
 * @brief Undo inline hook
 * @param [in] handler		The patch handler
 */
void ctest_unpatch(ctest_stub_t* handler);

/**
 * @}
 */

/************************************************************************/
/* internal interface                                                   */
/************************************************************************/

/**
 * @addtogroup Internal
 * @{
 */

/**
 * @internal
 * @def TEST_NOINLINE
 * @brief Prevents a function from being considered for inlining.
 * @note If the function does not have side-effects, there are optimizations
 *   other than inlining that causes function calls to be optimized away,
 *   although the function call is live.
 */
/**
 * @internal
 * @def TEST_NORETURN
 * @brief Define function that never return.
 */
/**
 * @internal
 * @def TEST_UNREACHABLE
 * @brief If control flow reaches the point of the TEST_UNREACHABLE, the
 *   program is undefined.
 */
#if defined(__GNUC__) || defined(__clang__)
#	define TEST_NOINLINE	__attribute__((noinline))
#	define TEST_NORETURN	__attribute__((noreturn))
#	define TEST_UNREACHABLE	__builtin_unreachable()
#elif defined(_MSC_VER)
#	define TEST_NOINLINE	__declspec(noinline)
#	define TEST_NORETURN	__declspec(noreturn)
#	define TEST_UNREACHABLE	__assume(0)
#else
#	define TEST_NOINLINE
#	define TEST_NORETURN
#	define TEST_UNREACHABLE
#endif

/**
 * @internal
 * @def TEST_MSVC_WARNNING_GUARD(exp, code)
 * @brief Disable warning for `code'.
 * @note This macro only works for MSVC.
 */
#if defined(_MSC_VER)
#	define TEST_MSVC_WARNNING_GUARD(exp, code)	\
		__pragma(warning(push))\
		__pragma(warning(disable : code))\
		exp\
		__pragma(warning(pop))
#else
#	define TEST_MSVC_WARNNING_GUARD(exp, code) \
		exp
#endif

/**
 * @internal
 * @def TEST_DEBUGBREAK
 * @brief Causes a breakpoint in your code, where the user will be prompted to
 *   run the debugger.
 */
#if defined(_MSC_VER)
#	define TEST_DEBUGBREAK		__debugbreak()
#elif !defined(__native_client__) \
	&& (defined(__clang__) || defined(__GNUC__)) && (defined(__x86_64__) || defined(__i386__))
#	define TEST_DEBUGBREAK		asm("int3")
#else
#	define TEST_DEBUGBREAK		*(volatile int*)NULL = 1
#endif

/**
 * @internal
 * @def TEST_PRIsize
 * @brief A correct format for print `size_t'
 */
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#	define TEST_PRIsize	"Iu"
#else
#	define TEST_PRIsize	"zu"
#endif

/**
 * @internal
 * @def TEST_ARG_COUNT
 * @brief Get the number of arguments
 */
#ifdef _MSC_VER // Microsoft compilers
#   define TEST_ARG_COUNT(...)  INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#   define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define INTERNAL_EXPAND(x) x
#   define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#else // Non-Microsoft compilers
#   define TEST_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#endif

#define TEST_EXPAND(x)		x
#define TEST_JOIN(a, b)		TEST_JOIN2(a, b)
#define TEST_JOIN2(a, b)	a##b

/**
 * @internal
 * @def TEST_INITIALIZER(f)
 * @brief Run the following code before main() invoke.
 */
#ifdef __cplusplus
#	define TEST_INITIALIZER(f) \
		void f(void); \
		struct f##_t_ { f##_t_(void) { f(); } }; static f##_t_ f##_; \
		void f(void)
#elif defined(_MSC_VER)
#	pragma section(".CRT$XCU",read)
#	define TEST_INITIALIZER2_(f,p) \
		void f(void); \
		__declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
		__pragma(comment(linker,"/include:" p #f "_")) \
		void f(void)
#	ifdef _WIN64
#		define TEST_INITIALIZER(f) TEST_INITIALIZER2_(f,"")
#	else
#		define TEST_INITIALIZER(f) TEST_INITIALIZER2_(f,"_")
#	endif
#elif defined(__GNUC__) || defined(__clang__) || defined(__llvm__)
#	define TEST_INITIALIZER(f) \
		void f(void) __attribute__((constructor)); \
		void f(void)
#else
#	error "INITIALIZER not support on your arch"
#endif

/**
 * @internal
 * @brief	A user defined assert template
 * @param[in] TYPE	Data type
 * @param[in] FMT	Print format
 * @param[in] OP	Compare operation
 * @param[in] CMP	Compare function
 * @param[in] a		Value a
 * @param[in] b		Value b
 * @param[in] u_fmt	User defined error message
 * @param[in] ...	User defined error message parameters
 */
#define ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, u_fmt, ...)	\
	do {\
		TYPE _a = (TYPE)(a); TYPE _b = (TYPE)(b);\
		if (CMP(_a, _b)) {\
			break;\
		}\
		printf("%s:%d:failure:" u_fmt "\n"\
			"            expected:    `%s' %s `%s'\n"\
			"              actual:    " FMT " vs " FMT "\n",\
			__FILE__, __LINE__, ##__VA_ARGS__, #a, #OP, #b, _a, _b);\
		ctest_internal_flush();\
		if (ctest_internal_break_on_failure()) {\
			TEST_DEBUGBREAK;\
		}\
		ctest_internal_assert_failure();\
	} TEST_MSVC_WARNNING_GUARD(while (0), 4127)

#define ASSERT_TEMPLATE_VA(...)									TEST_JOIN(ASSERT_TEMPLATE_VA_, TEST_ARG_COUNT(__VA_ARGS__))
#define ASSERT_TEMPLATE_VA_0(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_1(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_2(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_3(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_4(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_5(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_6(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_7(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_8(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_9(TYPE, FMT, OP, CMP, a, b, ...)		TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))

#define _ASSERT_INTERNAL_HELPER_EQ(a, b)		((a) == (b))
#define _ASSERT_INTERNAL_HELPER_NE(a, b)		((a) != (b))
#define _ASSERT_INTERNAL_HELPER_LT(a, b)		((a) < (b))
#define _ASSERT_INTERNAL_HELPER_LE(a, b)		((a) <= (b))
#define _ASSERT_INTERNAL_HELPER_GT(a, b)		((a) > (b))
#define _ASSERT_INTERNAL_HELPER_GE(a, b)		((a) >= (b))

typedef enum ctest_case_type
{
	ctest_case_type_suit,
	ctest_case_type_fixture,
	ctest_case_type_parameterized,
}ctest_case_type_t;

typedef struct ctest_list_node
{
	struct ctest_list_node*		p_after;				/**< next node */
	struct ctest_list_node*		p_before;				/**< previous node */
}ctest_list_node_t;

typedef struct ctest_map_node
{
	struct ctest_map_node*		__rb_parent_color;		/**< father node | color */
	struct ctest_map_node*		rb_right;				/**< right child node */
	struct ctest_map_node*		rb_left;				/**< left child node */
}ctest_map_node_t;

typedef void(*ctest_procedure_fn)(void);
typedef void(*ctest_parameterized_fn)(void*);

typedef struct ctest_case
{
	struct
	{
		ctest_list_node_t			queue;					/**< list node */
		ctest_map_node_t			table;					/**< map node */
	}node;

	struct
	{
		ctest_case_type_t			type;					/**< case type */
		unsigned					mask;					/**< internal mask */
		const char*					suit_name;				/**< suit name */
		const char*					case_name;				/**< case name */
	}info;

	struct
	{
		ctest_procedure_fn			setup;					/**< setup */
		ctest_procedure_fn			teardown;				/**< teardown */

		uintptr_t					body;					/**< test body */

		size_t						n_dat;					/**< parameterized data size */
		void*						p_dat;					/**< parameterized data */
	}stage;
}ctest_case_t;

/**
 * @internal
 * @brief Register test case
 * @param[in] test_case		Test case
 */
void ctest_register_case(ctest_case_t* test_case);

/**
 * @internal
 * @brief Set current test as failure
 * @note This function is available in setup stage and test body.
 * @warning Call this function in TearDown stage will cause assert.
 */
TEST_NORETURN void ctest_internal_assert_failure(void);
TEST_NORETURN void ctest_unwrap_assert_fail(const char *expr, const char *file, int line, const char *func);
void ctest_internal_flush(void);
int ctest_internal_break_on_failure(void);
int ctest_internal_assert_helper_str_eq(const char* a, const char* b);
int ctest_internal_assert_helper_float_eq(float a, float b);
int ctest_internal_assert_helper_float_le(float a, float b);
int ctest_internal_assert_helper_float_ge(float a, float b);
int ctest_internal_assert_helper_double_eq(double a, double b);
int ctest_internal_assert_helper_double_le(double a, double b);
int ctest_internal_assert_helper_double_ge(double a, double b);
size_t ctest_internal_parameterized_index(void);

const char* ctest_pretty_file(const char* file);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
