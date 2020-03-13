#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include "TEST.h"

#if defined(_MSC_VER)
#include <windows.h>
#	define GET_TID()						((unsigned long)GetCurrentThreadId())
#	define snprintf(str, size, fmt, ...)	_snprintf_s(str, size, _TRUNCATE, fmt, ##__VA_ARGS__)
#	define strdup(str)						_strdup(str)
#	define strncasecmp(s1, s2, n)			_strnicmp(s1, s2, n)
#elif defined(__linux__)
#include <pthread.h>
#	define GET_TID()						((unsigned long)pthread_self())
#else
#	define GET_TID()						0
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
	test_list_node_t*		head;				/** 头结点 */
	test_list_node_t*		tail;				/** 尾节点 */
	unsigned				size;				/** 节点数量 */
}test_list_t;

typedef struct test_ctx
{
	struct
	{
		test_list_t			case_list;			/** 用例列表 */
		unsigned long		tid;				/** 线程ID */
	}info;

	jmp_buf					jmpbuf;				/** 跳转地址 */

	struct
	{
		test_list_node_t*	cur_it;				/** 当前游标位置 */
		test_case_t*		cur_case;			/** 当前正在运行的用例 */
		size_t				cur_idx;			/** 当前游标位置 */
		int					flag_failed;		/** 标记是否失败 */
	}runtime;

	struct
	{
		unsigned			success;			/** 成功用例数 */
		unsigned			total;				/** 总运行用例数 */
	}counter;

	struct
	{
		char*				str;				/** 用例过滤器 */
		size_t				str_len;			/** 字符串长度 */
	}filter;

	char					match_buf[128];		/** 匹配缓冲区 */
}test_ctx_t;

static test_ctx_t			g_test_ctx;				/** 全局运行环境 */

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

static void _test_run_case(void)
{
	/* 判断是否需要运行 */
	if (g_test_ctx.filter.str != NULL)
	{
		snprintf(g_test_ctx.match_buf, sizeof(g_test_ctx.match_buf), "%s.%s",
			g_test_ctx.runtime.cur_case->data.cases[0].class_name,
			g_test_ctx.runtime.cur_case->data.cases[0].case_name);

		size_t str_orig_len = strlen(g_test_ctx.match_buf);
		size_t min_str_len = str_orig_len < g_test_ctx.filter.str_len ? str_orig_len : g_test_ctx.filter.str_len;

		if (strncasecmp(g_test_ctx.match_buf, g_test_ctx.filter.str, min_str_len) != 0)
		{
			return;
		}
	}

	g_test_ctx.runtime.flag_failed = 0;
	g_test_ctx.runtime.cur_idx = 0;
	g_test_ctx.counter.total++;

	printf("[ RUN      ] %s.%s\n",
		g_test_ctx.runtime.cur_case->data.cases[0].class_name,
		g_test_ctx.runtime.cur_case->data.cases[0].case_name);

	if (setjmp(g_test_ctx.jmpbuf) != 0)
	{
		/* 标记失败 */
		g_test_ctx.runtime.flag_failed = 1;
		/* 进入清理阶段 */
		if (g_test_ctx.runtime.cur_case->data.size == 1)
		{/* 不存在class时，直接跳出 */
			g_test_ctx.runtime.cur_idx = 1;
		}
		else if (g_test_ctx.runtime.cur_case->data.size == 3)
		{/* 存在class时，进行teardown阶段 */
			g_test_ctx.runtime.cur_idx = 2;
		}

		printf("[  FAILED  ] %s.%s\n",
			g_test_ctx.runtime.cur_case->data.cases[0].class_name,
			g_test_ctx.runtime.cur_case->data.cases[0].case_name);
	}

	for (;g_test_ctx.runtime.cur_idx < g_test_ctx.runtime.cur_case->data.size;
		g_test_ctx.runtime.cur_idx++)
	{
		g_test_ctx.runtime.cur_case->data.cases[g_test_ctx.runtime.cur_idx].test_fn();
	}

	if (!g_test_ctx.runtime.flag_failed)
	{
		printf("[       OK ] %s.%s\n",
			g_test_ctx.runtime.cur_case->data.cases[0].class_name,
			g_test_ctx.runtime.cur_case->data.cases[0].case_name);

		g_test_ctx.counter.success++;
	}
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

void test_optparse_init(test_optparse_t *options, char **argv)
{
	options->argv = argv;
	options->permute = 1;
	options->optind = 1;
	options->subopt = 0;
	options->optarg = 0;
	options->errmsg[0] = '\0';
}

void test_register_cases(test_case_t* cases)
{
	_test_init_ctx_once();
	_test_list_push_back(&g_test_ctx.info.case_list, &cases->node);
}

int test_run_tests(int argc, char* argv[])
{
	test_optparse_long_opt_t longopts[] = {
		{ "test_filter", 'f', OPTPARSE_OPTIONAL },
		{ 0, 0, 0 },
	};

	test_optparse_t options;
	test_optparse_init(&options, argv);

	int option;
	while ((option = test_optparse_long(&options, longopts, NULL)) != -1) {
		switch (option) {
		case 'f':
			g_test_ctx.filter.str = strdup(options.optarg);
			g_test_ctx.filter.str_len = strlen(g_test_ctx.filter.str);
			break;
		case '?':
			fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			goto fin;
		}
	}

	_test_init_ctx_once();
	memset(&g_test_ctx.counter, 0, sizeof(g_test_ctx.counter));

	printf("[==========] total %u tests registered.\n", g_test_ctx.info.case_list.size);

	g_test_ctx.runtime.cur_it = _test_list_begin(&g_test_ctx.info.case_list);
	for (; g_test_ctx.runtime.cur_it != NULL;
		g_test_ctx.runtime.cur_it = _test_list_next(&g_test_ctx.info.case_list, g_test_ctx.runtime.cur_it))
	{
		g_test_ctx.runtime.cur_case = CONTAINER_OF(g_test_ctx.runtime.cur_it, test_case_t, node);
		_test_run_case();
	}

	printf("[==========] %u tests case ran.\n", g_test_ctx.counter.total);
	printf("[  PASSED  ] %u tests.\n", g_test_ctx.counter.success);

	/* 清理资源 */
fin:
	if (g_test_ctx.filter.str != NULL)
	{
		free(g_test_ctx.filter.str);
		g_test_ctx.filter.str = NULL;
		g_test_ctx.filter.str_len = 0;
	}

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

/************************************************************************/
/* Argument Parser                                                      */
/************************************************************************/

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"


static int _test_optparse_is_dashdash(const char *arg)
{
	return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] == '\0';
}

static int _test_optparse_is_shortopt(const char *arg)
{
	return arg != 0 && arg[0] == '-' && arg[1] != '-' && arg[1] != '\0';
}

static int _test_optparse_is_longopt(const char *arg)
{
	return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] != '\0';
}

static int _test_optparse_longopts_end(const test_optparse_long_opt_t *longopts, int i)
{
	return !longopts[i].longname && !longopts[i].shortname;
}

static int _test_optparse_longopts_match(const char *longname, const char *option)
{
	const char *a = option, *n = longname;
	if (longname == 0)
		return 0;
	for (; *a && *n && *a != '='; a++, n++)
		if (*a != *n)
			return 0;
	return *n == '\0' && (*a == '\0' || *a == '=');
}

static char* _test_optparse_longopts_arg(char *option)
{
	for (; *option && *option != '='; option++);
	if (*option == '=')
		return option + 1;
	else
		return 0;
}

static void _test_optparse_from_long(const test_optparse_long_opt_t *longopts, char *optstring)
{
	char *p = optstring;
	int i;
	for (i = 0; !_test_optparse_longopts_end(longopts, i); i++) {
		if (longopts[i].shortname) {
			int a;
			*p++ = longopts[i].shortname;
			for (a = 0; a < (int)longopts[i].argtype; a++)
				*p++ = ':';
		}
	}
	*p = '\0';
}

static void _test_optparse_permute(test_optparse_t *options, int index)
{
	char *nonoption = options->argv[index];
	int i;
	for (i = index; i < options->optind - 1; i++)
		options->argv[i] = options->argv[i + 1];
	options->argv[options->optind - 1] = nonoption;
}

static int _test_optparse_argtype(const char *optstring, char c)
{
	int count = OPTPARSE_NONE;
	if (c == ':')
		return -1;
	for (; *optstring && c != *optstring; optstring++);
	if (!*optstring)
		return -1;
	if (optstring[1] == ':')
		count += optstring[2] == ':' ? 2 : 1;
	return count;
}

static int _test_optparse_error(test_optparse_t *options, const char *msg, const char *data)
{
	unsigned p = 0;
	const char *sep = " -- '";
	while (*msg)
		options->errmsg[p++] = *msg++;
	while (*sep)
		options->errmsg[p++] = *sep++;
	while (p < sizeof(options->errmsg) - 2 && *data)
		options->errmsg[p++] = *data++;
	options->errmsg[p++] = '\'';
	options->errmsg[p++] = '\0';
	return '?';
}

static int _test_optparse(test_optparse_t *options, const char *optstring)
{
	int type;
	char *next;
	char *option = options->argv[options->optind];
	options->errmsg[0] = '\0';
	options->optopt = 0;
	options->optarg = 0;
	if (option == 0) {
		return -1;
	}
	else if (_test_optparse_is_dashdash(option)) {
		options->optind++; /* consume "--" */
		return -1;
	}
	else if (!_test_optparse_is_shortopt(option)) {
		if (options->permute) {
			int index = options->optind++;
			int r = _test_optparse(options, optstring);
			_test_optparse_permute(options, index);
			options->optind--;
			return r;
		}
		else {
			return -1;
		}
	}
	option += options->subopt + 1;
	options->optopt = option[0];
	type = _test_optparse_argtype(optstring, option[0]);
	next = options->argv[options->optind + 1];
	switch (type) {
	case -1: {
		char str[2] = { 0, 0 };
		str[0] = option[0];
		options->optind++;
		return _test_optparse_error(options, OPTPARSE_MSG_INVALID, str);
	}
	case OPTPARSE_NONE:
		if (option[1]) {
			options->subopt++;
		}
		else {
			options->subopt = 0;
			options->optind++;
		}
		return option[0];
	case OPTPARSE_REQUIRED:
		options->subopt = 0;
		options->optind++;
		if (option[1]) {
			options->optarg = option + 1;
		}
		else if (next != 0) {
			options->optarg = next;
			options->optind++;
		}
		else {
			char str[2] = { 0, 0 };
			str[0] = option[0];
			options->optarg = 0;
			return _test_optparse_error(options, OPTPARSE_MSG_MISSING, str);
		}
		return option[0];
	case OPTPARSE_OPTIONAL:
		options->subopt = 0;
		options->optind++;
		if (option[1])
			options->optarg = option + 1;
		else
			options->optarg = 0;
		return option[0];
	}
	return 0;
}

static int _test_optparse_long_fallback(test_optparse_t *options, const test_optparse_long_opt_t *longopts, int *longindex)
{
	int result;
	char optstring[96 * 3 + 1]; /* 96 ASCII printable characters */
	_test_optparse_from_long(longopts, optstring);
	result = _test_optparse(options, optstring);
	if (longindex != 0) {
		*longindex = -1;
		if (result != -1) {
			int i;
			for (i = 0; !_test_optparse_longopts_end(longopts, i); i++)
				if (longopts[i].shortname == options->optopt)
					*longindex = i;
		}
	}
	return result;
}

int test_optparse_long(test_optparse_t *options, const test_optparse_long_opt_t *longopts, int *longindex)
{
	int i;
	char *option = options->argv[options->optind];
	if (option == 0) {
		return -1;
	}
	else if (_test_optparse_is_dashdash(option)) {
		options->optind++; /* consume "--" */
		return -1;
	}
	else if (_test_optparse_is_shortopt(option)) {
		return _test_optparse_long_fallback(options, longopts, longindex);
	}
	else if (!_test_optparse_is_longopt(option)) {
		if (options->permute) {
			int index = options->optind++;
			int r = test_optparse_long(options, longopts, longindex);
			_test_optparse_permute(options, index);
			options->optind--;
			return r;
		}
		else {
			return -1;
		}
	}

	/* Parse as long option. */
	options->errmsg[0] = '\0';
	options->optopt = 0;
	options->optarg = 0;
	option += 2; /* skip "--" */
	options->optind++;
	for (i = 0; !_test_optparse_longopts_end(longopts, i); i++) {
		const char *name = longopts[i].longname;
		if (_test_optparse_longopts_match(name, option)) {
			char *arg;
			if (longindex)
				*longindex = i;
			options->optopt = longopts[i].shortname;
			arg = _test_optparse_longopts_arg(option);
			if (longopts[i].argtype == OPTPARSE_NONE && arg != 0) {
				return _test_optparse_error(options, OPTPARSE_MSG_TOOMANY, name);
			} if (arg != 0) {
				options->optarg = arg;
			}
			else if (longopts[i].argtype == OPTPARSE_REQUIRED) {
				options->optarg = options->argv[options->optind];
				if (options->optarg == 0)
					return _test_optparse_error(options, OPTPARSE_MSG_MISSING, name);
				else
					options->optind++;
			}
			return options->optopt;
		}
	}
	return _test_optparse_error(options, OPTPARSE_MSG_INVALID, option);
}

/************************************************************************/
/* LOG                                                                  */
/************************************************************************/

static const char* _log_getfilename(const char* file)
{
	const char* pos = file;

	for (; *file; ++file)
	{
		if (*file == '\\' || *file == '/')
		{
			pos = file + 1;
		}
	}
	return pos;
}

void test_log(const char* file, const char* func, int line, const char* fmt, ...)
{
	printf("[%s:%d %s] ", _log_getfilename(file), line, func);

	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	printf("\n");
}
