#ifndef __ETEST_ETEST_H__
#define __ETEST_ETEST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

/************************************************************************/
/* TEST                                                                 */
/************************************************************************/

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

#ifdef _MSC_VER // Microsoft compilers
#   define GET_ARG_COUNT(...)  INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#   define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define INTERNAL_EXPAND(x) x
#   define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#else // Non-Microsoft compilers
#   define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#endif

#define TEST_EXPAND(x)		x
#define TEST_JOIN(a, b)		TEST_JOIN2(a, b)
#define TEST_JOIN2(a, b)	a##b

/**
 * @brief Setup test suit
 */
#define TEST_CLASS_SETUP(suit_name)	\
	static void TEST_CLASS_SETUP_##suit_name(void)

/**
 * @brief Teardown test suit
 */
#define TEST_CLASS_TEAREDOWN(suit_name)	\
	static void TEST_CLASS_TEARDOWN_##suit_name(void)

/**
 * 测试用例
 * @param suit_name		suit name
 * @param case_name		case name
 */
#define TEST_F(suit_name, case_name)	\
	static void TEST_##suit_name##_##case_name(void);\
	TEST_INITIALIZER(TEST_INIT_##suit_name##_##case_name) {\
		static etest_case_t _case_##suit_name##_##case_name = {\
			{ { NULL, NULL }, { NULL, NULL, NULL } },\
			{ 0, #suit_name, #case_name,\
				{\
					TEST_CLASS_SETUP_##suit_name,\
					TEST_##suit_name##_##case_name,\
					TEST_CLASS_TEARDOWN_##suit_name\
				}\
			},\
		};\
		etest_register_case(&_case_##suit_name##_##case_name);\
	}\
	static void TEST_##suit_name##_##case_name(void)

/**
 * 测试用例
 * @param suit_name		suit name
 * @param case_name		case name
 */
#define TEST(suit_name, case_name)	\
	static void TEST_##suit_name##_##case_name(void);\
	TEST_INITIALIZER(TEST_INIT_##case_name) {\
		static etest_case_t _case_##suit_name##_##case_name = {\
			{ { NULL, NULL }, { NULL, NULL, NULL } },\
			{ 0, #suit_name, #case_name,\
				{\
					NULL, TEST_##suit_name##_##case_name, NULL\
				}\
			},\
		};\
		etest_register_case(&_case_##suit_name##_##case_name);\
	}\
	static void TEST_##suit_name##_##case_name(void)

#define ASSERT(x)	\
	((void)((x) || (etest_internal_assert_fail(#x, __FILE__, __LINE__, __FUNCTION__),0)))

#if defined(_MSC_VER)
#	define TEST_DEBUGBREAK		__debugbreak()
#elif !defined(__native_client__) && (defined(__clang__) || defined(__GNUC__)) && (defined(__x86_64__) || defined(__i386__))
#	define TEST_DEBUGBREAK		asm("int3")
#else
#	define TEST_DEBUGBREAK		*(volatile int*)NULL = 1
#endif

#define ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, u_fmt, ...)	\
	do {\
		TYPE _a = a; TYPE _b = b;\
		if (CMP(_a, _b)) {\
			break;\
		}\
		printf("%s:%d:failure:" u_fmt "\n"\
			"            expected:    `%s' %s `%s'\n"\
			"              actual:    " FMT " vs " FMT "\n",\
			__FILE__, __LINE__, ##__VA_ARGS__, #a, #OP, #b, _a, _b);\
		etest_internal_flush();\
		if (etest_internal_break_on_failure()) {\
			TEST_DEBUGBREAK;\
		}\
		etest_internal_set_as_failure();\
	} TEST_MSVC_WARNNING_GUARD(while (0), 4127)

#define ASSERT_TEMPLATE_VA(...)									TEST_JOIN(ASSERT_TEMPLATE_VA_, GET_ARG_COUNT(__VA_ARGS__))
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

#define _ASSERT_INTERNAL_HELPER_EQ(a, b)						((a) == (b))
#define _ASSERT_INTERNAL_HELPER_NE(a, b)						((a) != (b))
#define _ASSERT_INTERNAL_HELPER_LT(a, b)						((a) < (b))
#define _ASSERT_INTERNAL_HELPER_LE(a, b)						((a) <= (b))
#define _ASSERT_INTERNAL_HELPER_GT(a, b)						((a) > (b))
#define _ASSERT_INTERNAL_HELPER_GE(a, b)						((a) >= (b))

#define ASSERT_EQ_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_D32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%"PRId32, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_U32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%"PRIu32, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_X32(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%#010"PRIx32, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_D64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%"PRId64, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_U64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%"PRIu64, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_X64(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%#018"PRIx64, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%zu", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%zu", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%zu", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%zu", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%zu", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_SIZE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%zu", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_PTR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_INT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(int, "%d", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_UINT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned, "%u", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_LONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(long, "%l", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_ULONG(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(unsigned long, "%lu", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

#define ASSERT_EQ_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", ==, etest_internal_assert_helper_float_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", !=, !etest_internal_assert_helper_float_eq, a, b, ##__VA_ARGS__)
#define ASSERT_LT_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", <,  !etest_internal_assert_helper_float_ge, a, b, ##__VA_ARGS__)
#define ASSERT_LE_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", <=, etest_internal_assert_helper_float_le,  a, b, ##__VA_ARGS__)
#define ASSERT_GT_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", >,  !etest_internal_assert_helper_float_le, a, b, ##__VA_ARGS__)
#define ASSERT_GE_FLOAT(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", >=, etest_internal_assert_helper_float_ge,  a, b, ##__VA_ARGS__)

#define ASSERT_EQ_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", ==, etest_internal_assert_helper_double_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", !=, !etest_internal_assert_helper_double_eq, a, b, ##__VA_ARGS__)
#define ASSERT_LT_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", <,  !etest_internal_assert_helper_double_ge, a, b, ##__VA_ARGS__)
#define ASSERT_LE_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", <=, etest_internal_assert_helper_double_le,  a, b, ##__VA_ARGS__)
#define ASSERT_GT_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", >,  !etest_internal_assert_helper_double_le, a, b, ##__VA_ARGS__)
#define ASSERT_GE_DOUBLE(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", >=, etest_internal_assert_helper_double_ge,  a, b, ##__VA_ARGS__)

#define ASSERT_EQ_STR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const char*, "%s", ==, etest_internal_assert_helper_str_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_STR(a, b, ...)		ASSERT_TEMPLATE_VA(__VA_ARGS__)(const char*, "%s", !=, !etest_internal_assert_helper_str_eq, a, b, ##__VA_ARGS__)

typedef struct etest_list_node
{
	struct etest_list_node*		p_after;				/**< next node */
	struct etest_list_node*		p_before;				/**< previous node */
}etest_list_node_t;

typedef struct etest_map_node
{
	struct etest_map_node*		__rb_parent_color;		/**< father node | color */
	struct etest_map_node*		rb_right;				/**< right child node */
	struct etest_map_node*		rb_left;				/**< left child node */
}etest_map_node_t;

typedef void(*etest_procedure_fn)(void);

typedef struct etest_case
{
	struct
	{
		etest_list_node_t		queue;					/**< 侵入式节点 */
		etest_map_node_t		table;					/**< 侵入式节点 */
	}node;

	struct
	{
		unsigned long			mask;					/**< 内部标记 */
		const char*				suit_name;				/**< 用例集名称 */
		const char*				case_name;				/**< 用例名称 */
		etest_procedure_fn		proc[3];				/**< 用例体 */
	}data;
}etest_case_t;

/**
 * @brief Register test case
 * @param[in] test_case		Test case
 */
void etest_register_case(etest_case_t* test_case);

/**
 * @brief Run all test cases
 * @param[in] argc	The number of arguments
 * @param[in] argv	The argument list
 * @return			The number of failure test
 */
int etest_run_tests(int argc, char* argv[]);

/**
 * @brief Get current running suit name
 * @return			The suit name
 */
const char* etest_get_current_suit_name(void);

/**
 * @brief Get current running case name
 * @return			The case name
 */
const char* etest_get_current_case_name(void);

/**
 * @brief Skip current test case
 */
TEST_NORETURN void etest_skip_test(void);

TEST_NORETURN void etest_internal_set_as_failure(void);
TEST_NORETURN void etest_internal_assert_fail(const char *expr, const char *file, int line, const char *func);
void etest_internal_flush(void);
int etest_internal_break_on_failure(void);
int etest_internal_assert_helper_str_eq(const char* a, const char* b);
int etest_internal_assert_helper_float_eq(float a, float b);
int etest_internal_assert_helper_float_le(float a, float b);
int etest_internal_assert_helper_float_ge(float a, float b);
int etest_internal_assert_helper_double_eq(double a, double b);
int etest_internal_assert_helper_double_le(double a, double b);
int etest_internal_assert_helper_double_ge(double a, double b);

/************************************************************************/
/* LOG                                                                  */
/************************************************************************/

/**
 * @brief 简单日志
 */
#define TEST_LOG(fmt, ...)	\
	printf("[%s:%d %s] " fmt "\n", etest_pretty_file(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

const char* etest_pretty_file(const char* file);

/************************************************************************/
/* time                                                                 */
/************************************************************************/

typedef struct etest_timestamp
{
	uint64_t	sec;		/** seconds */
	uint64_t	usec;		/** microseconds */
}etest_timestamp_t;

/**
 * @brief Monotonic time since some unspecified starting point
 * @param ts		etest_timestamp_t*
 * @return		0 if success, otherwise failure
 */
int etest_timestamp_get(etest_timestamp_t* ts);

/**
 * @brief Compare timestamp
 * @param t1	timestamp t1
 * @param t2	timestamp t2
 * @param dif	diff
 * @return		-1 if t1 < t2; 1 if t1 > t2; 0 if t1 == t2
 */
int etest_timestamp_dif(const etest_timestamp_t* t1, const etest_timestamp_t* t2, etest_timestamp_t* dif);

/************************************************************************/
/* inline hook                                                          */
/************************************************************************/
struct etest_stub;
typedef struct etest_stub etest_stub_t;

/**
 * @brief 进行函数patch
 * @param fn_orig	原函数
 * @param fn_stub	替换函数
 * @return			替换句柄，可随后用于取消patch
 */
etest_stub_t* etest_patch(uintptr_t fn_orig, uintptr_t fn_stub);

/**
 * @brief 取消函数替换
 * @param handler	替换句柄
 */
void etest_unpatch(etest_stub_t* handler);

#ifdef __cplusplus
}
#endif
#endif
