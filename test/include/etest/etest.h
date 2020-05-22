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
#elif defined(_MSC_VER)
#	define TEST_NOINLINE	__declspec(noinline)
#else
#	define TEST_NOINLINE
#endif

/**
* 初始化用例集
*/
#define TEST_CLASS_SETUP(test_fixture)	\
	static void TEST_CLASS_SETUP_##test_fixture(void)

/**
* 卸载用例集
*/
#define TEST_CLASS_TEAREDOWN(test_fixture)	\
	static void TEST_CLASS_TEARDOWN_##test_fixture(void)

/**
* 测试用例
* @param class_name	所属用例集
* @param case_name	用例名
*/
#define TEST_F(test_fixture, test_name)	\
	static void TEST_##test_fixture##_##test_name(void);\
	TEST_INITIALIZER(TEST_INIT_##test_fixture##_##test_name) {\
		static etest_case_t item = {\
			{ { NULL, NULL }, { NULL, NULL, NULL } },\
			{ 0, #test_fixture, #test_name,\
				{\
					TEST_CLASS_SETUP_##test_fixture,\
					TEST_##test_fixture##_##test_name,\
					TEST_CLASS_TEARDOWN_##test_fixture\
				}\
			},\
		};\
		etest_register_case(&item);\
	}\
	static void TEST_##test_fixture##_##test_name(void)

/**
* 测试用例
* @param case_name	用例名
*/
#define TEST(test_case_name, test_name)	\
	static void TEST_##test_case_name##_##test_name(void);\
	TEST_INITIALIZER(TEST_INIT_##test_name) {\
		static etest_case_t item = {\
			{ { NULL, NULL }, { NULL, NULL, NULL } },\
			{ 0, #test_case_name, #test_name,\
				{\
					NULL, TEST_##test_case_name##_##test_name, NULL\
				}\
			},\
		};\
		etest_register_case(&item);\
	}\
	static void TEST_##test_case_name##_##test_name(void)

#define ASSERT(x)	\
	((void)((x) || (etest_assert_fail(#x, __FILE__, __LINE__, __FUNCTION__),0)))

#define ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b)	\
	do {\
		TYPE _a = a; TYPE _b = b;\
		if (CMP(_a, _b)) {\
			break;\
		}\
		printf("%s:%d: failure\n"\
			"            expected:    `%s' %s `%s'\n"\
			"              actual:    " FMT " vs " FMT "\n", __FILE__, __LINE__, #a, #OP, #b, _a, _b);\
		if (etest_break_on_failure()) {\
			*(volatile int*)NULL = 1;\
		} else {\
			etest_set_as_failure();\
		}\
	} while (etest_always_zero())

#define _ASSERT_HELPER_EQ(a, b)		((a) == (b))
#define _ASSERT_HELPER_NE(a, b)		((a) != (b))
#define _ASSERT_HELPER_LT(a, b)		((a) < (b))
#define _ASSERT_HELPER_LE(a, b)		((a) <= (b))
#define _ASSERT_HELPER_GT(a, b)		((a) > (b))
#define _ASSERT_HELPER_GE(a, b)		((a) >= (b))

#define ASSERT_EQ_PTR(a, b)			ASSERT_TEMPLATE(const void*, "%p", ==, _ASSERT_HELPER_EQ, a, b)
#define ASSERT_NE_PTR(a, b)			ASSERT_TEMPLATE(const void*, "%p", !=, _ASSERT_HELPER_NE, a, b)
#define ASSERT_LT_PTR(a, b)			ASSERT_TEMPLATE(const void*, "%p", <, _ASSERT_HELPER_LT, a, b)
#define ASSERT_LE_PTR(a, b)			ASSERT_TEMPLATE(const void*, "%p", <=, _ASSERT_HELPER_LE, a, b)
#define ASSERT_GT_PTR(a, b)			ASSERT_TEMPLATE(const void*, "%p", >, _ASSERT_HELPER_GT, a, b)
#define ASSERT_GE_PTR(a, b)			ASSERT_TEMPLATE(const void*, "%p", >=, _ASSERT_HELPER_GE, a, b)

#define ASSERT_EQ_D32(a, b)			ASSERT_TEMPLATE(int32_t, "%"PRId32, ==, _ASSERT_HELPER_EQ, a, b)
#define ASSERT_NE_D32(a, b)			ASSERT_TEMPLATE(int32_t, "%"PRId32, !=, _ASSERT_HELPER_NE, a, b)
#define ASSERT_LT_D32(a, b)			ASSERT_TEMPLATE(int32_t, "%"PRId32, <, _ASSERT_HELPER_LT, a, b)
#define ASSERT_LE_D32(a, b)			ASSERT_TEMPLATE(int32_t, "%"PRId32, <=, _ASSERT_HELPER_LE, a, b)
#define ASSERT_GT_D32(a, b)			ASSERT_TEMPLATE(int32_t, "%"PRId32, >, _ASSERT_HELPER_GT, a, b)
#define ASSERT_GE_D32(a, b)			ASSERT_TEMPLATE(int32_t, "%"PRId32, >=, _ASSERT_HELPER_GE, a, b)

#define ASSERT_EQ_U32(a, b)			ASSERT_TEMPLATE(uint32_t, "%"PRIu32, ==, _ASSERT_HELPER_EQ, a, b)
#define ASSERT_NE_U32(a, b)			ASSERT_TEMPLATE(uint32_t, "%"PRIu32, !=, _ASSERT_HELPER_NE, a, b)
#define ASSERT_LT_U32(a, b)			ASSERT_TEMPLATE(uint32_t, "%"PRIu32, <, _ASSERT_HELPER_LT, a, b)
#define ASSERT_LE_U32(a, b)			ASSERT_TEMPLATE(uint32_t, "%"PRIu32, <=, _ASSERT_HELPER_LE, a, b)
#define ASSERT_GT_U32(a, b)			ASSERT_TEMPLATE(uint32_t, "%"PRIu32, >, _ASSERT_HELPER_GT, a, b)
#define ASSERT_GE_U32(a, b)			ASSERT_TEMPLATE(uint32_t, "%"PRIu32, >=, _ASSERT_HELPER_GE, a, b)

#define ASSERT_EQ_X32(a, b)			ASSERT_TEMPLATE(uint32_t, "%#010"PRIx32, ==, _ASSERT_HELPER_EQ, a, b)
#define ASSERT_NE_X32(a, b)			ASSERT_TEMPLATE(uint32_t, "%#010"PRIx32, !=, _ASSERT_HELPER_NE, a, b)
#define ASSERT_LT_X32(a, b)			ASSERT_TEMPLATE(uint32_t, "%#010"PRIx32, <, _ASSERT_HELPER_LT, a, b)
#define ASSERT_LE_X32(a, b)			ASSERT_TEMPLATE(uint32_t, "%#010"PRIx32, <=, _ASSERT_HELPER_LE, a, b)
#define ASSERT_GT_X32(a, b)			ASSERT_TEMPLATE(uint32_t, "%#010"PRIx32, >, _ASSERT_HELPER_GT, a, b)
#define ASSERT_GE_X32(a, b)			ASSERT_TEMPLATE(uint32_t, "%#010"PRIx32, >=, _ASSERT_HELPER_GE, a, b)

#define ASSERT_EQ_D64(a, b)			ASSERT_TEMPLATE(int64_t, "%"PRId64, ==, _ASSERT_HELPER_EQ, a, b)
#define ASSERT_NE_D64(a, b)			ASSERT_TEMPLATE(int64_t, "%"PRId64, !=, _ASSERT_HELPER_NE, a, b)
#define ASSERT_LT_D64(a, b)			ASSERT_TEMPLATE(int64_t, "%"PRId64, <, _ASSERT_HELPER_LT, a, b)
#define ASSERT_LE_D64(a, b)			ASSERT_TEMPLATE(int64_t, "%"PRId64, <=, _ASSERT_HELPER_LE, a, b)
#define ASSERT_GT_D64(a, b)			ASSERT_TEMPLATE(int64_t, "%"PRId64, >, _ASSERT_HELPER_GT, a, b)
#define ASSERT_GE_D64(a, b)			ASSERT_TEMPLATE(int64_t, "%"PRId64, >=, _ASSERT_HELPER_GE, a, b)

#define ASSERT_EQ_U64(a, b)			ASSERT_TEMPLATE(uint64_t, "%"PRIu64, ==, _ASSERT_HELPER_EQ, a, b)
#define ASSERT_NE_U64(a, b)			ASSERT_TEMPLATE(uint64_t, "%"PRIu64, !=, _ASSERT_HELPER_NE, a, b)
#define ASSERT_LT_U64(a, b)			ASSERT_TEMPLATE(uint64_t, "%"PRIu64, <, _ASSERT_HELPER_LT, a, b)
#define ASSERT_LE_U64(a, b)			ASSERT_TEMPLATE(uint64_t, "%"PRIu64, <=, _ASSERT_HELPER_LE, a, b)
#define ASSERT_GT_U64(a, b)			ASSERT_TEMPLATE(uint64_t, "%"PRIu64, >, _ASSERT_HELPER_GT, a, b)
#define ASSERT_GE_U64(a, b)			ASSERT_TEMPLATE(uint64_t, "%"PRIu64, >=, _ASSERT_HELPER_GE, a, b)

#define ASSERT_EQ_X64(a, b)			ASSERT_TEMPLATE(uint64_t, "%#018"PRIx64, ==, _ASSERT_HELPER_EQ, a, b)
#define ASSERT_NE_X64(a, b)			ASSERT_TEMPLATE(uint64_t, "%#018"PRIx64, !=, _ASSERT_HELPER_NE, a, b)
#define ASSERT_LT_X64(a, b)			ASSERT_TEMPLATE(uint64_t, "%#018"PRIx64, <, _ASSERT_HELPER_LT, a, b)
#define ASSERT_LE_X64(a, b)			ASSERT_TEMPLATE(uint64_t, "%#018"PRIx64, <=, _ASSERT_HELPER_LE, a, b)
#define ASSERT_GT_X64(a, b)			ASSERT_TEMPLATE(uint64_t, "%#018"PRIx64, >, _ASSERT_HELPER_GT, a, b)
#define ASSERT_GE_X64(a, b)			ASSERT_TEMPLATE(uint64_t, "%#018"PRIx64, >=, _ASSERT_HELPER_GE, a, b)

#define ASSERT_EQ_DBL(a, b)			ASSERT_TEMPLATE(double, "%f", ==, etest_assert_helper_double_eq, a, b)
#define ASSERT_NE_DBL(a, b)			ASSERT_TEMPLATE(double, "%f", !=, !etest_assert_helper_double_eq, a, b)
#define ASSERT_LT_DBL(a, b)			ASSERT_TEMPLATE(double, "%f", <, !etest_assert_helper_double_ge, a, b)
#define ASSERT_LE_DBL(a, b)			ASSERT_TEMPLATE(double, "%f", <=, etest_assert_helper_double_le, a, b)
#define ASSERT_GT_DBL(a, b)			ASSERT_TEMPLATE(double, "%f", >, !etest_assert_helper_double_le, a, b)
#define ASSERT_GE_DBL(a, b)			ASSERT_TEMPLATE(double, "%f", >=, etest_assert_helper_double_ge, a, b)

#define ASSERT_EQ_SIZE(a, b)		ASSERT_TEMPLATE(size_t, "%zu", ==, _ASSERT_HELPER_EQ, a, b)
#define ASSERT_NE_SIZE(a, b)		ASSERT_TEMPLATE(size_t, "%zu", !=, _ASSERT_HELPER_NE, a, b)
#define ASSERT_LT_SIZE(a, b)		ASSERT_TEMPLATE(size_t, "%zu", <, _ASSERT_HELPER_LT, a, b)
#define ASSERT_LE_SIZE(a, b)		ASSERT_TEMPLATE(size_t, "%zu", <=, _ASSERT_HELPER_LE, a, b)
#define ASSERT_GT_SIZE(a, b)		ASSERT_TEMPLATE(size_t, "%zu", >, _ASSERT_HELPER_GT, a, b)
#define ASSERT_GE_SIZE(a, b)		ASSERT_TEMPLATE(size_t, "%zu", >=, _ASSERT_HELPER_GE, a, b)

#define ASSERT_EQ_STR(a, b)			ASSERT_TEMPLATE(const char*, "%s", ==, etest_assert_helper_str_eq, a, b)
#define ASSERT_NE_STR(a, b)			ASSERT_TEMPLATE(const char*, "%s", !=, !etest_assert_helper_str_eq, a, b)

typedef struct etest_list_node
{
	struct etest_list_node*		p_after;				/** 下一节点 */
	struct etest_list_node*		p_before;				/** 上一节点 */
}etest_list_node_t;

typedef struct etest_map_node
{
	struct etest_map_node*		__rb_parent_color;		/** 父节点|颜色 */
	struct etest_map_node*		rb_right;				/** 右子节点 */
	struct etest_map_node*		rb_left;				/** 左子节点 */
}etest_map_node_t;

typedef void(*etest_procedure_fn)(void);

typedef struct etest_case
{
	struct
	{
		etest_list_node_t		queue;					/** 侵入式节点 */
		etest_map_node_t		table;					/** 侵入式节点 */
	}node;

	struct
	{
		unsigned long			mask;					/** 内部标记 */
		const char*				class_name;				/** 用例集名称 */
		const char*				case_name;				/** 用例名称 */
		etest_procedure_fn		proc[3];				/** 用例体 */
	}data;
}etest_case_t;

/**
* 注册用例
* @param cases	用例列表。全局可访问
* @param size	列表长度
*/
void etest_register_case(etest_case_t* data);

/**
* 运行测试用例
*/
int etest_run_tests(int argc, char* argv[]);

void etest_assert_fail(const char *expr, const char *file, int line, const char *func);
void etest_set_as_failure(void);
int etest_break_on_failure(void);

int etest_assert_helper_str_eq(const char* a, const char* b);
int etest_assert_helper_double_eq(double a, double b);
int etest_assert_helper_double_le(double a, double b);
int etest_assert_helper_double_ge(double a, double b);
int etest_always_zero(void);

/************************************************************************/
/* LOG                                                                  */
/************************************************************************/

/**
* 简单日志
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
 * monotonic time since some unspecified starting point
 * @param ts		etest_timestamp_t*
 * @return		0 if success, otherwise failure
 */
int etest_timestamp_get(etest_timestamp_t* ts);

/**
 * compare timestamp
 * @param t1		timestamp t1
 * @param t2		timestamp t2
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
 * 进行函数patch
 * @param fn_orig	原函数
 * @param fn_stub	替换函数
 * @return			替换句柄，可随后用于取消patch
 */
etest_stub_t* etest_patch(uintptr_t fn_orig, uintptr_t fn_stub);

/**
 * 取消函数替换
 * @param handler	替换句柄
 */
void etest_unpatch(etest_stub_t* handler);

#ifdef __cplusplus
}
#endif
#endif
