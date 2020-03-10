#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "TEST.h"

#if defined(_MSC_VER)
#include <windows.h>
#	define GET_TID()	((unsigned long)GetCurrentThreadId())
#elif defined(__linux__)
#include <pthread.h>
#	define GET_TID()	((unsigned long)pthread_self())
#else
#	define GET_TID()	0
#endif

#define CONTAINER_OF(ptr, TYPE, member)	\
	((TYPE*)((char*)(ptr) - (size_t)&((TYPE*)0)->member))

#define JUDGE_GENERIC_TEMPLATE(_cmp, _op, _fmt, _l, _a, _r, _b, _file, _line)	\
	do {\
		if (GET_TID() != g_test_ctx.info.tid) {\
			printf("%s:%d: excepton\n"\
					"\tyou are not allowed to call assert functions outside test thread.\n",\
					_file, _line);\
			assert(0);\
		}\
		if (_cmp) {\
			break;\
		}\
		printf("%s:%d: failure\n"\
			"\texpected:\t`%s` " _op " `%s`\n"\
			"\t  actual:\t" _fmt " vs " _fmt "\n",\
			_file, _line, _l, _r, _a, _b);\
		longjmp(g_test_ctx.jmpbuf, 1);\
	} while (0)

typedef struct test_list
{
	test_list_node_t*	head;		/** 头结点 */
	test_list_node_t*	tail;		/** 尾节点 */
	unsigned			size;		/** 节点数量 */
}test_list_t;

typedef struct test_ctx
{
	struct
	{
		test_list_t		case_list;	/** 用例列表 */
		unsigned long	tid;		/** 线程ID */
	}info;

	jmp_buf				jmpbuf;		/** 跳转地址 */

	struct
	{
		unsigned		success;	/** 成功用例数 */
		unsigned		total;		/** 总运行用例数 */
	}counter;
}test_ctx_t;

static test_ctx_t		g_test_ctx;	/** 全局运行环境 */

static void _test_list_set_once(test_list_t* handler, test_list_node_t* node)
{
	handler->head = node;
	handler->tail = node;
	node->p_after = NULL;
	node->p_before = NULL;
	handler->size = 1;
}

static void _test_list_push_back(test_list_t* handler, test_list_node_t* node)
{
	if (handler->head == NULL)
	{
		_test_list_set_once(handler, node);
		return;
	}

	node->p_after = NULL;
	node->p_before = handler->tail;
	handler->tail->p_after = node;
	handler->tail = node;
	handler->size++;
}

static test_list_node_t* _test_list_begin(const test_list_t* handler)
{
	return handler->head;
}

static test_list_node_t* _test_list_next(const test_list_t* handler, const test_list_node_t* node)
{
	return node->p_after;
}

static void _test_run_case_normal(test_case_t* whole_case)
{
	printf("[ RUN      ] %s.%s\n",
		whole_case->data.cases[0].class_name,
		whole_case->data.cases[0].case_name);

	unsigned i;
	for (i = 0; i < whole_case->data.size; i++)
	{
		whole_case->data.cases[i].test_fn();
	}

	printf("[       OK ] %s.%s\n",
		whole_case->data.cases[0].class_name,
		whole_case->data.cases[0].case_name);

	g_test_ctx.counter.success++;
}

static void _test_run_case_error(test_case_t* whole_case)
{
	// do nothing
	printf("[  FAILED  ] %s.%s\n",
		whole_case->data.cases[0].class_name,
		whole_case->data.cases[0].case_name);
}

static void _test_run_case(test_case_t* whole_case)
{
	g_test_ctx.counter.total++;

	int ret = setjmp(g_test_ctx.jmpbuf);
	if (ret == 0)
	{
		_test_run_case_normal(whole_case);
		return;
	}

	_test_run_case_error(whole_case);
}

static void _test_init_ctx_once(void)
{
	static int once_flag = 1;
	if (once_flag)
	{
		once_flag = 0;
		memset(&g_test_ctx, 0, sizeof(g_test_ctx));

		g_test_ctx.info.tid = GET_TID();
	}
}

void test_register_cases(test_case_t* cases)
{
	_test_init_ctx_once();
	_test_list_push_back(&g_test_ctx.info.case_list, &cases->node);
}

int test_run_tests(int argc, char* argv[])
{
	_test_init_ctx_once();
	memset(&g_test_ctx.counter, 0, sizeof(g_test_ctx.counter));

	printf("[==========] running %u tests.\n", g_test_ctx.info.case_list.size);

	test_list_node_t* it = _test_list_begin(&g_test_ctx.info.case_list);
	for (; it != NULL; it = _test_list_next(&g_test_ctx.info.case_list, it))
	{
		test_case_t* real_case = CONTAINER_OF(it, test_case_t, node);
		_test_run_case(real_case);
	}

	printf("[==========] %u tests case ran.\n", g_test_ctx.counter.total);
	printf("[  PASSED  ] %u tests.\n", g_test_ctx.counter.success);

	return 0;
}

void test_assert_num_eq(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a == b, "==", "%lld", s_a, a, s_b, b, file, line);
}

void test_assert_num_ne(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a != b, "!=", "%lld", s_a, a, s_b, b, file, line);
}

void test_assert_num_lt(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a < b, "<", "%lld", s_a, a, s_b, b, file, line);
}

void test_assert_num_le(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a <= b, "<=", "%lld", s_a, a, s_b, b, file, line);
}

void test_assert_num_gt(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a > b, ">", "%lld", s_a, a, s_b, b, file, line);
}

void test_assert_num_ge(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a >= b, ">=", "%lld", s_a, a, s_b, b, file, line);
}

void test_assert_ptr_eq(void* a, void* b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a == b, "==", "%p", s_a, a, s_b, b, file, line);
}

void test_assert_ptr_ne(void* a, void* b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a != b, "!=", "%p", s_a, a, s_b, b, file, line);
}

void test_assert_str_eq(const char* a, const char* b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(strcmp(a, b) == 0, "!=", "%p", s_a, a, s_b, b, file, line);
}

void test_assert_str_ne(const char* a, const char* b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(strcmp(a, b) != 0, "!=", "%p", s_a, a, s_b, b, file, line);
}

void test_assert_flt_eq(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a == b, "==", "%f", s_a, a, s_b, b, file, line);
}

void test_assert_flt_ne(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a != b, "!=", "%f", s_a, a, s_b, b, file, line);
}

void test_assert_flt_lt(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a < b, "<", "%f", s_a, a, s_b, b, file, line);
}

void test_assert_flt_le(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a <= b, "<=", "%f", s_a, a, s_b, b, file, line);
}

void test_assert_flt_gt(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a > b, ">", "%f", s_a, a, s_b, b, file, line);
}

void test_assert_flt_ge(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a >= b, ">=", "%f", s_a, a, s_b, b, file, line);
}
