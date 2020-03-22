#ifndef __TEST_H__
#define __TEST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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
* ��ʼ��������
*/
#define TEST_CLASS_SETUP(test_fixture)	\
	static void TEST_CLASS_SETUP_##test_fixture(void)

/**
* ж��������
*/
#define TEST_CLASS_TEAREDOWN(test_fixture)	\
	static void TEST_CLASS_TEARDOWN_##test_fixture(void)

/**
* ��������
* @param class_name	����������
* @param case_name	������
*/
#define TEST_F(test_fixture, test_name)	\
	static void TEST_##test_fixture##_##test_name(void);\
	TEST_INITIALIZER(TEST_INIT_##test_fixture##_##test_name) {\
		static test_case_item_t cases[] = {\
			{ #test_fixture, #test_name, test_case_type_setup, TEST_CLASS_SETUP_##test_fixture },\
			{ #test_fixture, #test_name, test_case_type_normal, TEST_##test_fixture##_##test_name },\
			{ #test_fixture, #test_name, test_case_type_teardown, TEST_CLASS_TEARDOWN_##test_fixture },\
		};\
		static test_case_t item = { { NULL, NULL }, 0, { sizeof(cases) / sizeof(cases[0]), cases } };\
		test_register_case(&item);\
	}\
	static void TEST_##test_fixture##_##test_name(void)

/**
* ��������
* @param case_name	������
*/
#define TEST(test_case_name, test_name)	\
	static void TEST_##test_case_name##_##test_name(void);\
	TEST_INITIALIZER(TEST_INIT_##test_name) {\
		static test_case_item_t cases[] = {\
			{ #test_case_name, #test_name, test_case_type_normal, TEST_##test_case_name##_##test_name },\
		};\
		static test_case_t item = { { NULL, NULL }, 0, { sizeof(cases) / sizeof(cases[0]), cases } };\
		test_register_case(&item);\
	}\
	static void TEST_##test_case_name##_##test_name(void)

#define ASSERT(x)	\
	((void)((x) || (test_assert_fail(#x, __FILE__, __LINE__, __func__),0)))

#define ASSERT_PTR_EQ(a, b)		test_assert_ptr_eq(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_PTR_NE(a, b)		test_assert_ptr_ne(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_STR_EQ(a, b)		test_assert_str_eq(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_STR_NE(a, b)		test_assert_str_ne(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_NUM_EQ(a, b)		test_assert_num_eq(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_NUM_NE(a, b)		test_assert_num_ne(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_NUM_LT(a, b)		test_assert_num_lt(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_NUM_LE(a, b)		test_assert_num_le(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_NUM_GT(a, b)		test_assert_num_gt(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_NUM_GE(a, b)		test_assert_num_ge(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_FLT_EQ(a, b)		test_assert_flt_eq(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_FLT_NE(a, b)		test_assert_flt_ne(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_FLT_LT(a, b)		test_assert_flt_lt(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_FLT_LE(a, b)		test_assert_flt_le(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_FLT_GT(a, b)		test_assert_flt_gt(a, b, #a, #b, __FILE__, __LINE__)
#define ASSERT_FLT_GE(a, b)		test_assert_flt_ge(a, b, #a, #b, __FILE__, __LINE__)

typedef enum test_case_type
{
	test_case_type_setup,							/** ��ʼ�� */
	test_case_type_normal,							/** ���� */
	test_case_type_teardown,						/** ȥ��ʼ�� */
}test_case_type_t;

typedef struct test_list_node
{
	struct test_list_node*	p_after;				/** ��һ�ڵ� */
	struct test_list_node*	p_before;				/** ��һ�ڵ� */
}test_list_node_t;

typedef struct test_case_item
{
	const char*				class_name;				/** ���������� */
	const char*				case_name;				/** �������� */
	test_case_type_t		type;					/** �������� */
	void					(*test_fn)(void);		/** ������ַ */
}test_case_item_t;

typedef struct test_case
{
	test_list_node_t		node;					/** ����ʽ�ڵ� */
	unsigned long			mask;					/** �ڲ���� */
	struct
	{
		unsigned			size;					/** �������� */
		test_case_item_t*	cases;					/** �����б� */
	}data;
}test_case_t;

/**
* ע������
* @param cases	�����б�ȫ�ֿɷ���
* @param size	�б���
*/
void test_register_case(test_case_t* data);

/**
* ���в�������
*/
int test_run_tests(int argc, char* argv[]);

void test_assert_fail(const char *expr, const char *file, int line, const char *func);
void test_assert_ptr_eq(void* a, void* b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_ptr_ne(void* a, void* b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_str_eq(const char* a, const char* b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_str_ne(const char* a, const char* b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_num_eq(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_num_ne(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_num_lt(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_num_le(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_num_gt(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_num_ge(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_flt_eq(double a, double b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_flt_ne(double a, double b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_flt_lt(double a, double b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_flt_le(double a, double b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_flt_gt(double a, double b, const char* s_a, const char* s_b, const char* file, int line);
void test_assert_flt_ge(double a, double b, const char* s_a, const char* s_b, const char* file, int line);

/************************************************************************/
/* LOG                                                                  */
/************************************************************************/

/**
* ����־
*/
#define TEST_LOG(fmt, ...)	\
	printf("[%s:%d %s] " fmt "\n", test_pretty_file(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

const char* test_pretty_file(const char* file);

#ifdef __cplusplus
}
#endif
#endif
