#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include "etest/etest.h"

/************************************************************************/
/* map                                                                  */
/************************************************************************/

#define	RB_RED		0
#define	RB_BLACK	1
#define __rb_color(pc)     ((uintptr_t)(pc) & 1)
#define __rb_is_black(pc)  __rb_color(pc)
#define __rb_is_red(pc)    (!__rb_color(pc))
#define __rb_parent(pc)    ((etest_map_node_t*)(pc & ~3))
#define rb_color(rb)       __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb)      __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb)    __rb_is_black((rb)->__rb_parent_color)
#define rb_parent(r)   ((etest_map_node_t*)((uintptr_t)((r)->__rb_parent_color) & ~3))

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define RB_EMPTY_NODE(node)  \
	((node)->__rb_parent_color == (etest_map_node_t*)(node))

typedef int(*etest_map_cmp_fn)(const etest_map_node_t* key1, const etest_map_node_t* key2, void* arg);

typedef struct etest_map
{
	etest_map_node_t*		rb_root;	/** 根元素节点 */

	struct
	{
		etest_map_cmp_fn	cmp;		/** 对比函数 */
		void*				arg;		/** 自定义参数 */
	}cmp;

	size_t					size;		/** 当前元素容量 */
}etest_map_t;
#define ETEST_MAP_INITIALIZER(fn, arg)		{ NULL, { fn, arg }, 0 }

static void _etest_map_link_node(etest_map_node_t* node, etest_map_node_t* parent, etest_map_node_t** rb_link)
{
	node->__rb_parent_color = parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
	return;
}

static etest_map_node_t* rb_red_parent(etest_map_node_t *red)
{
	return red->__rb_parent_color;
}

static void rb_set_parent_color(etest_map_node_t *rb, etest_map_node_t *p, int color)
{
	rb->__rb_parent_color = (etest_map_node_t*)((uintptr_t)p | color);
}

static void __rb_change_child(etest_map_node_t* old_node, etest_map_node_t* new_node,
	etest_map_node_t* parent, etest_map_t* root)
{
	if (parent)
	{
		if (parent->rb_left == old_node)
		{
			parent->rb_left = new_node;
		}
		else
		{
			parent->rb_right = new_node;
		}
	}
	else
	{
		root->rb_root = new_node;
	}
}

/*
* Helper function for rotations:
* - old's parent and color get assigned to new
* - old gets assigned new as a parent and 'color' as a color.
*/
static void __rb_rotate_set_parents(etest_map_node_t* old, etest_map_node_t* new_node,
	etest_map_t* root, int color)
{
	etest_map_node_t* parent = rb_parent(old);
	new_node->__rb_parent_color = old->__rb_parent_color;
	rb_set_parent_color(old, new_node, color);
	__rb_change_child(old, new_node, parent, root);
}

static void _etest_map_insert(etest_map_node_t* node, etest_map_t* root)
{
	etest_map_node_t* parent = rb_red_parent(node), *gparent, *tmp;

	while (1) {
		/*
		* Loop invariant: node is red
		*
		* If there is a black parent, we are done.
		* Otherwise, take some corrective action as we don't
		* want a red root or two consecutive red nodes.
		*/
		if (!parent) {
			rb_set_parent_color(node, NULL, RB_BLACK);
			break;
		}
		else if (rb_is_black(parent))
			break;

		gparent = rb_red_parent(parent);

		tmp = gparent->rb_right;
		if (parent != tmp) {	/* parent == gparent->rb_left */
			if (tmp && rb_is_red(tmp)) {
				/*
				* Case 1 - color flips
				*
				*       G            g
				*      / \          / \
				*     p   u  -->   P   U
				*    /            /
				*   n            n
				*
				* However, since g's parent might be red, and
				* 4) does not allow this, we need to recurse
				* at g.
				*/
				rb_set_parent_color(tmp, gparent, RB_BLACK);
				rb_set_parent_color(parent, gparent, RB_BLACK);
				node = gparent;
				parent = rb_parent(node);
				rb_set_parent_color(node, parent, RB_RED);
				continue;
			}

			tmp = parent->rb_right;
			if (node == tmp) {
				/*
				* Case 2 - left rotate at parent
				*
				*      G             G
				*     / \           / \
				*    p   U  -->    n   U
				*     \           /
				*      n         p
				*
				* This still leaves us in violation of 4), the
				* continuation into Case 3 will fix that.
				*/
				parent->rb_right = tmp = node->rb_left;
				node->rb_left = parent;
				if (tmp)
					rb_set_parent_color(tmp, parent,
					RB_BLACK);
				rb_set_parent_color(parent, node, RB_RED);
				parent = node;
				tmp = node->rb_right;
			}

			/*
			* Case 3 - right rotate at gparent
			*
			*        G           P
			*       / \         / \
			*      p   U  -->  n   g
			*     /                 \
			*    n                   U
			*/
			gparent->rb_left = tmp;  /* == parent->rb_right */
			parent->rb_right = gparent;
			if (tmp)
				rb_set_parent_color(tmp, gparent, RB_BLACK);
			__rb_rotate_set_parents(gparent, parent, root, RB_RED);
			break;
		}
		else {
			tmp = gparent->rb_left;
			if (tmp && rb_is_red(tmp)) {
				/* Case 1 - color flips */
				rb_set_parent_color(tmp, gparent, RB_BLACK);
				rb_set_parent_color(parent, gparent, RB_BLACK);
				node = gparent;
				parent = rb_parent(node);
				rb_set_parent_color(node, parent, RB_RED);
				continue;
			}

			tmp = parent->rb_left;
			if (node == tmp) {
				/* Case 2 - right rotate at parent */
				parent->rb_left = tmp = node->rb_right;
				node->rb_right = parent;
				if (tmp)
					rb_set_parent_color(tmp, parent,
					RB_BLACK);
				rb_set_parent_color(parent, node, RB_RED);
				parent = node;
				tmp = node->rb_left;
			}

			/* Case 3 - left rotate at gparent */
			gparent->rb_right = tmp;  /* == parent->rb_left */
			parent->rb_left = gparent;
			if (tmp)
				rb_set_parent_color(tmp, gparent, RB_BLACK);
			__rb_rotate_set_parents(gparent, parent, root, RB_RED);
			break;
		}
	}
}

static void _etest_map_insert_color(etest_map_node_t* node, etest_map_t* root)
{
	_etest_map_insert(node, root);
}

static int etest_map_insert(etest_map_t* handler, etest_map_node_t* node)
{
	etest_map_node_t **new_node = &(handler->rb_root), *parent = NULL;

	/* Figure out where to put new node */
	while (*new_node)
	{
		int result = handler->cmp.cmp(node, *new_node, handler->cmp.arg);

		parent = *new_node;
		if (result < 0)
		{
			new_node = &((*new_node)->rb_left);
		}
		else if (result > 0)
		{
			new_node = &((*new_node)->rb_right);
		}
		else
		{
			return -1;
		}
	}

	handler->size++;
	_etest_map_link_node(node, parent, new_node);
	_etest_map_insert_color(node, handler);

	return 0;
}

static etest_map_node_t* etest_map_begin(const etest_map_t* handler)
{
	etest_map_node_t* n = handler->rb_root;

	if (!n)
		return NULL;
	while (n->rb_left)
		n = n->rb_left;
	return n;
}

static etest_map_node_t* etest_map_next(const etest_map_t* handler, const etest_map_node_t* node)
{
	etest_map_node_t* parent;

	if (RB_EMPTY_NODE(node))
		return NULL;

	/*
	* If we have a right-hand child, go down and then left as far
	* as we can.
	*/
	if (node->rb_right) {
		node = node->rb_right;
		while (node->rb_left)
			node = node->rb_left;
		return (etest_map_node_t *)node;
	}

	/*
	* No right-hand children. Everything down and left is smaller than us,
	* so any 'next' node must be in the general direction of our parent.
	* Go up the tree; any time the ancestor is a right-hand child of its
	* parent, keep going up. First time it's a left-hand child of its
	* parent, said parent is our 'next' node.
	*/
	while ((parent = rb_parent(node)) != NULL && node == parent->rb_right)
		node = parent;

	return parent;
}

/************************************************************************/
/* argument parser                                                      */
/************************************************************************/

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"

typedef enum test_optparse_argtype {
	OPTPARSE_NONE,
	OPTPARSE_REQUIRED,
	OPTPARSE_OPTIONAL
}test_optparse_argtype_t;

typedef struct test_optparse_long_opt {
	const char*				longname;
	int						shortname;
	test_optparse_argtype_t	argtype;
}test_optparse_long_opt_t;

typedef struct test_optparse {
	char**					argv;
	int						permute;
	int						optind;
	int						optopt;
	char*					optarg;
	char					errmsg[64];
	size_t					subopt;
}test_optparse_t;

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
	option += (options->subopt + 1);
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

static int test_optparse_long(test_optparse_t *options, const test_optparse_long_opt_t *longopts, int *longindex)
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

static void test_optparse_init(test_optparse_t *options, char **argv)
{
	options->argv = argv;
	options->permute = 1;
	options->optind = 1;
	options->subopt = 0;
	options->optarg = 0;
	options->errmsg[0] = '\0';
}

/************************************************************************/
/* list                                                                 */
/************************************************************************/

typedef struct test_list
{
	etest_list_node_t*		head;							/** 头结点 */
	etest_list_node_t*		tail;							/** 尾节点 */
	unsigned				size;							/** 节点数量 */
}test_list_t;
#define TEST_LIST_INITIALIZER		{ NULL, NULL, 0 }

static void _test_list_set_once(test_list_t* handler, etest_list_node_t* node)
{
	handler->head = node;
	handler->tail = node;
	node->p_after = NULL;
	node->p_before = NULL;
	handler->size = 1;
}

static void _test_list_push_back(test_list_t* handler, etest_list_node_t* node)
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

static etest_list_node_t* _test_list_begin(const test_list_t* handler)
{
	return handler->head;
}

static etest_list_node_t* _test_list_next(const test_list_t* handler, const etest_list_node_t* node)
{
	return node->p_after;
}

static unsigned _test_list_size(const test_list_t* handler)
{
	return handler->size;
}

static void _test_list_erase(test_list_t* handler, etest_list_node_t* node)
{
	handler->size--;

	/* 唯一节点 */
	if (handler->head == node && handler->tail == node)
	{
		handler->head = NULL;
		handler->tail = NULL;
		return;
	}

	if (handler->head == node)
	{
		node->p_after->p_before = NULL;
		handler->head = node->p_after;
		return;
	}

	if (handler->tail == node)
	{
		node->p_before->p_after = NULL;
		handler->tail = node->p_before;
		return;
	}

	node->p_before->p_after = node->p_after;
	node->p_after->p_before = node->p_before;
	return;
}

/************************************************************************/
/* test                                                                 */
/************************************************************************/

#if defined(_MSC_VER)
#	include <windows.h>
#	include <time.h>
#	define GET_TID()						((unsigned long)GetCurrentThreadId())
#	define snprintf(str, size, fmt, ...)	_snprintf_s(str, size, _TRUNCATE, fmt, ##__VA_ARGS__)
#	ifndef strdup
#		define strdup(str)					_strdup(str)
#	endif
#	define strncasecmp(s1, s2, n)			_strnicmp(s1, s2, n)
#	define sscanf(str, fmt, ...)			sscanf_s(str, fmt, ##__VA_ARGS__)
#	define COLOR_GREEN(str)					str
#	define COLOR_RED(str)					str
#	define COLOR_YELLO(str)					str
#elif defined(__linux__)
#	include <sys/time.h>
#	include <pthread.h>
#	define GET_TID()						((unsigned long)pthread_self())
#	define COLOR_GREEN(str)					"\033[32m" str "\033[0m"
#	define COLOR_RED(str)					"\033[31m" str "\033[0m"
#	define COLOR_YELLO(str)					"\033[33m" str "\033[0m"
#else
#	define GET_TID()						0
#	define COLOR_GREEN(str)					str
#	define COLOR_RED(str)					str
#	define COLOR_YELLO(str)					str
#endif

#define MASK_FAILED							(0x01 << 0x00)
#define SET_MASK(val, mask)					do { (val) |= (mask); } while (0)
#define HAS_MASK(val, mask)					((val) & (mask))

#define ARRAY_SIZE(arr)						(sizeof(arr) / sizeof(arr[0]))

/**
* 一秒内的毫秒数
*/
#define USEC_IN_SEC							(1 * 1000 * 1000)

#define CONTAINER_OF(ptr, TYPE, member)	\
	((TYPE*)((char*)(ptr) - (size_t)&((TYPE*)0)->member))

#define JUDGE_GENERIC_TEMPLATE(_cmp, _op, _fmt, _l, _a, _r, _b, _file, _line)	\
	do {\
		if (GET_TID() != g_test_ctx.info.tid) {\
			printf("%s:%d: excepton\n"\
					"\tyou are not allowed to call assert functions outside test thread.\n",\
					_file, _line);\
			ASSERT(0);\
		}\
		if (_cmp) {\
			break;\
		}\
		printf("%s:%d: failure\n"\
			"            expected:    `%s` " _op " `%s`\n"\
			"              actual:    " _fmt " vs " _fmt "\n",\
			_file, _line, _l, _r, _a, _b);\
		if (g_test_ctx.mask.break_on_failure) {\
			abort();\
		}\
		longjmp(g_test_ctx2.jmpbuf, 1);\
	} while (0)

typedef int test_bool;
#define test_true		1
#define test_false		0

typedef struct test_ctx
{
	struct
	{
		test_list_t			case_list;						/** 用例表 */
		etest_map_t			case_table;						/** 用例表 */
		unsigned long		tid;							/** 线程ID */
	}info;

	struct
	{
		unsigned long long	seed;							/** 随机数 */
		etest_list_node_t*	cur_it;							/** 当前游标位置 */
		etest_case_t*		cur_case;						/** 当前正在运行的用例 */
		size_t				cur_idx;						/** 当前游标位置 */
	}runtime;

	struct
	{
		etest_timestamp_t	tv_case_start;					/** 用例开始时间 */
		etest_timestamp_t	tv_case_end;					/** 结束时间 */

		etest_timestamp_t	tv_total_start;					/** 总开始时间 */
		etest_timestamp_t	tv_total_end;					/** 总结束时间 */

		etest_timestamp_t	tv_diff;						/** 时间差值 */
	}timestamp;

	struct
	{
		struct
		{
			unsigned		disabled;						/** 关闭数量 */
			unsigned		total;							/** 总运行用例数 */
			unsigned		success;						/** 成功用例数 */
			unsigned		failed;							/** 失败用例数 */
		}result;

		struct
		{
			unsigned		repeat;							/** 重复次数 */
			unsigned		repeated;						/** 已经重复的次数 */
		}repeat;
	}counter;

	struct
	{
		unsigned			break_on_failure : 1;			/** 失败时生成断点 */
		unsigned			print_time : 1;					/** 是否输出用例执行时间 */
		unsigned			also_run_disabled_tests : 1;	/** 允许执行被关闭的用例 */
		unsigned			shuffle : 1;					/** 测试用例执行顺序随机 */
	}mask;

	struct
	{
		char**				postive_patterns;				/** 正向匹配 */
		char**				negative_patterns;				/** 反向匹配 */
		size_t				n_negative;						/** 正向数量 */
		size_t				n_postive;						/** 反向数量 */
	}filter;
}test_ctx_t;

typedef struct test_ctx2
{
	char					strbuf[128];					/** 字符缓冲区 */
	jmp_buf					jmpbuf;							/** 跳转地址 */
}test_ctx2_t;

static int _etest_on_cmp_case(const etest_map_node_t* key1, const etest_map_node_t* key2, void* arg);
static test_ctx2_t			g_test_ctx2;					// 不需要初始化
static test_ctx_t			g_test_ctx = {
	{ TEST_LIST_INITIALIZER, ETEST_MAP_INITIALIZER(_etest_on_cmp_case, NULL), 0 },							// .info
	{ 0, NULL, NULL, 0 },									// .runtime
	{ { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },	// .timestamp
	{ { 0, 0, 0, 0 }, { 1, 0 } },							// .counter
	{ 0, 1, 0, 0 },											// .mask
	{ NULL, NULL, 0, 0 },									// .filter
};

static int _etest_on_cmp_case(const etest_map_node_t* key1, const etest_map_node_t* key2, void* arg)
{
	const etest_case_t* t_case_1 = CONTAINER_OF(key1, etest_case_t, node.table);
	const etest_case_t* t_case_2 = CONTAINER_OF(key2, etest_case_t, node.table);

	int ret;
	if ((ret = strcmp(t_case_1->data.class_name, t_case_2->data.class_name)) != 0)
	{
		return ret;
	}
	return strcmp(t_case_1->data.case_name, t_case_2->data.case_name);
}

static void _test_srand(unsigned long long s)
{
	g_test_ctx.runtime.seed = s - 1;
}

static unsigned long _test_rand(void)
{
	g_test_ctx.runtime.seed = 6364136223846793005ULL * g_test_ctx.runtime.seed + 1;
	return g_test_ctx.runtime.seed >> 33;
}

#if defined(_MSC_VER)
LARGE_INTEGER getFILETIMEoffset()
{
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;

	return t;
}
#endif

int etest_timestamp_get(etest_timestamp_t* ts)
{
#if defined(_MSC_VER)

	LARGE_INTEGER           t;
	FILETIME            f;
	double                  microseconds;
	static LARGE_INTEGER    offset;
	static double           frequencyToMicroseconds;
	static int              initialized = 0;
	static BOOL             usePerformanceCounter = 0;

	if (!initialized) {
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
		if (usePerformanceCounter)
		{
			QueryPerformanceCounter(&offset);
			frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
		}
		else
		{
			offset = getFILETIMEoffset();
			frequencyToMicroseconds = 10.;
		}
	}
	if (usePerformanceCounter)
	{
		QueryPerformanceCounter(&t);
	}
	else
	{
		GetSystemTimeAsFileTime(&f);
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = (LONGLONG)microseconds;
	ts->sec = t.QuadPart / 1000000;
	ts->usec = t.QuadPart % 1000000;
	return 0;

#else
	struct timespec tmp_ts;
	if (clock_gettime(CLOCK_MONOTONIC, &tmp_ts) < 0)
	{
		return -1;
	}

	ts->sec = tmp_ts.tv_sec;
	ts->usec = tmp_ts.tv_nsec / 1000;
	return 0;
#endif
}

int etest_timestamp_dif(const etest_timestamp_t* t1, const etest_timestamp_t* t2, etest_timestamp_t* dif)
{
	etest_timestamp_t tmp_dif;
	const etest_timestamp_t* large_t = t1->sec > t2->sec ? t1 : (t1->sec < t2->sec ? t2 : (t1->usec > t2->usec ? t1 : t2));
	const etest_timestamp_t* little_t = large_t == t1 ? t2 : t1;

	tmp_dif.sec = large_t->sec - little_t->sec;
	if (large_t->usec < little_t->usec)
	{
		tmp_dif.usec = little_t->usec - large_t->usec;
		tmp_dif.sec--;
	}
	else
	{
		tmp_dif.usec = large_t->usec - little_t->usec;
	}

	if (dif != NULL)
	{
		*dif = tmp_dif;
	}

	if (tmp_dif.sec == 0 && tmp_dif.usec == 0)
	{
		return 0;
	}
	return t1 == little_t ? -1 : 1;
}

/**
* 检查str是否符合pat
* @return		bool
*/
static test_bool _test_pattern_matches_string(const char* pat, const char* str)
{
	switch (*pat)
	{
	case '\0':
		return *str == '\0';
	case '?':
		return *str != '\0' && _test_pattern_matches_string(pat + 1, str + 1);
	case '*':
		return (*str != '\0' && _test_pattern_matches_string(pat, str + 1)) || _test_pattern_matches_string(pat + 1, str);
	default:
		return *pat == *str && _test_pattern_matches_string(pat + 1, str + 1);
	}
}

static test_bool _test_check_pattern(const char* str)
{
	size_t i;
	for (i = 0; i < g_test_ctx.filter.n_negative; i++)
	{
		if (_test_pattern_matches_string(g_test_ctx.filter.negative_patterns[i], str))
		{
			return test_false;
		}
	}

	if (g_test_ctx.filter.n_postive == 0)
	{
		return test_true;
	}

	for (i = 0; i < g_test_ctx.filter.n_postive; i++)
	{
		if (_test_pattern_matches_string(g_test_ctx.filter.postive_patterns[i], str))
		{
			return test_true;
		}
	}

	return test_false;
}

static test_bool _test_check_disable(const char* name)
{
	return !g_test_ctx.mask.also_run_disabled_tests && (strncmp("DISABLED_", name, 9) == 0);
}

/**
* run test case.
* the target case was set to `g_test_ctx.runtime.cur_case`
*/
static void _test_run_case(void)
{
	snprintf(g_test_ctx2.strbuf, sizeof(g_test_ctx2.strbuf), "%s.%s",
		g_test_ctx.runtime.cur_case->data.class_name,
		g_test_ctx.runtime.cur_case->data.case_name);

	/* 判断是否需要运行 */
	if (g_test_ctx.filter.postive_patterns != NULL && !_test_check_pattern(g_test_ctx2.strbuf))
	{
		return;
	}
	if (_test_check_disable(g_test_ctx.runtime.cur_case->data.case_name))
	{
		g_test_ctx.counter.result.disabled++;
		return;
	}

	printf(COLOR_GREEN("[ RUN      ]") " %s\n", g_test_ctx2.strbuf);
	g_test_ctx.runtime.cur_idx = 0;
	g_test_ctx.counter.result.total++;

	if (setjmp(g_test_ctx2.jmpbuf) != 0)
	{
		/* 标记失败 */
		SET_MASK(g_test_ctx.runtime.cur_case->data.mask, MASK_FAILED);

		/* 进入清理阶段 */
		goto procedure_teardown;
	}

	if (g_test_ctx.runtime.cur_case->data.proc[0] != NULL)
	{
		g_test_ctx.runtime.cur_case->data.proc[0]();
	}

	/* 记录开始时间 */
	ASSERT(etest_timestamp_get(&g_test_ctx.timestamp.tv_case_start) == 0);
	if (g_test_ctx.runtime.cur_case->data.proc[1] != NULL)
	{
		g_test_ctx.runtime.cur_case->data.proc[1]();
	}

procedure_teardown:
	ASSERT(etest_timestamp_get(&g_test_ctx.timestamp.tv_case_end) == 0);
	if (g_test_ctx.runtime.cur_case->data.proc[2] != NULL)
	{
		g_test_ctx.runtime.cur_case->data.proc[2]();
	}

	etest_timestamp_dif(&g_test_ctx.timestamp.tv_case_start, &g_test_ctx.timestamp.tv_case_end, &g_test_ctx.timestamp.tv_diff);

	const char* prefix_str = COLOR_RED("[  FAILED  ]");
	if (!HAS_MASK(g_test_ctx.runtime.cur_case->data.mask, MASK_FAILED))
	{
		prefix_str = COLOR_GREEN("[       OK ]");
		g_test_ctx.counter.result.success++;
	}
	else
	{
		g_test_ctx.counter.result.failed++;
	}

	printf("%s %s", prefix_str, g_test_ctx2.strbuf);
	if (g_test_ctx.mask.print_time)
	{
		printf(" (%llu ms)", (unsigned long long)g_test_ctx.timestamp.tv_diff.sec * 1000 + g_test_ctx.timestamp.tv_diff.usec / 1000);
	}
	printf("\n");
}

static void _test_reset_all_test(void)
{
	memset(&g_test_ctx.counter.result, 0, sizeof(g_test_ctx.counter.result));
	memset(&g_test_ctx.timestamp, 0, sizeof(g_test_ctx.timestamp));

	etest_list_node_t* it = _test_list_begin(&g_test_ctx.info.case_list);
	for (; it != NULL; it = _test_list_next(&g_test_ctx.info.case_list, it))
	{
		etest_case_t* case_data = CONTAINER_OF(it, etest_case_t, node.queue);
		case_data->data.mask = 0;
	}

	g_test_ctx.info.tid = GET_TID();
}

static void _test_show_report(void)
{
	etest_timestamp_dif(&g_test_ctx.timestamp.tv_total_start, &g_test_ctx.timestamp.tv_total_end, &g_test_ctx.timestamp.tv_diff);

	printf(COLOR_GREEN("[==========]") " %u/%u test case%s ran.",
		g_test_ctx.counter.result.total,
		_test_list_size(&g_test_ctx.info.case_list),
		g_test_ctx.counter.result.total > 1 ? "s" : "");
	if (g_test_ctx.mask.print_time)
	{
		printf(" (%llu ms total)", (unsigned long long)g_test_ctx.timestamp.tv_diff.sec * 1000 + g_test_ctx.timestamp.tv_diff.usec / 1000);
	}
	printf("\n");

	if (g_test_ctx.counter.result.disabled != 0)
	{
		printf(COLOR_GREEN("[ DISABLED ]") " %u test%s.\n",
			g_test_ctx.counter.result.disabled,
			g_test_ctx.counter.result.disabled > 1 ? "s" : "");
	}

	printf(COLOR_GREEN("[  PASSED  ]") " %u test%s.\n",
		g_test_ctx.counter.result.success,
		g_test_ctx.counter.result.success > 1 ? "s" : "");
	if (g_test_ctx.counter.result.failed == 0)
	{
		return;
	}

	printf(COLOR_RED("[  FAILED  ]")" %u test%s, listed below:\n", g_test_ctx.counter.result.failed, g_test_ctx.counter.result.failed > 1 ? "s" : "");
	etest_list_node_t* it = _test_list_begin(&g_test_ctx.info.case_list);
	for (; it != NULL; it = _test_list_next(&g_test_ctx.info.case_list, it))
	{
		etest_case_t* case_data = CONTAINER_OF(it, etest_case_t, node.queue);
		if (!HAS_MASK(case_data->data.mask, MASK_FAILED))
		{
			continue;
		}

		snprintf(g_test_ctx2.strbuf, sizeof(g_test_ctx2.strbuf), "%s.%s",
			case_data->data.class_name, case_data->data.case_name);
		printf(COLOR_RED("[  FAILED  ]")" %s\n", g_test_ctx2.strbuf);
	}
}

static void _etest_setup_arg_pattern(const char* user_pattern)
{
	int flag_allow_negative = 1;
	size_t number_of_patterns = 1;
	size_t number_of_negative = 0;
	size_t user_pattern_size = 0;
	while (user_pattern[user_pattern_size] != '\0')
	{
		if (user_pattern[user_pattern_size] == '-' && flag_allow_negative)
		{/* 当遇到`-`时，若之前未遇到`:`，则不作为negative项 */
			flag_allow_negative = 0;
			number_of_negative++;
		}
		else if (user_pattern[user_pattern_size] == ':')
		{
			flag_allow_negative = 1;
			number_of_patterns++;
		}
		user_pattern_size++;
	}
	user_pattern_size++;

	size_t prefix_size = sizeof(void*) * number_of_patterns;
	size_t malloc_size = prefix_size + user_pattern_size;
	g_test_ctx.filter.postive_patterns = malloc(malloc_size);
	if (g_test_ctx.filter.postive_patterns == NULL)
	{
		return;
	}
	g_test_ctx.filter.negative_patterns = g_test_ctx.filter.postive_patterns + (number_of_patterns - number_of_negative);
	memcpy((char*)g_test_ctx.filter.postive_patterns + prefix_size, user_pattern, user_pattern_size);

	char* str_it = (char*)g_test_ctx.filter.postive_patterns + prefix_size;
	do 
	{
		while (*str_it == ':')
		{
			*str_it = '\0';
			str_it++;
		}

		if (*str_it == '\0')
		{
			return;
		}

		if (*str_it == '-')
		{
			*str_it = '\0';
			str_it++;

			g_test_ctx.filter.negative_patterns[g_test_ctx.filter.n_negative] = str_it;
			g_test_ctx.filter.n_negative++;
			continue;
		}

		g_test_ctx.filter.postive_patterns[g_test_ctx.filter.n_postive] = str_it;
		g_test_ctx.filter.n_postive++;
	} while ((str_it = strchr(str_it + 1, ':')) != NULL);
}

static void _etest_list_tests(void)
{
	unsigned c_class = 0;
	unsigned c_test = 0;
	const char* last_class_name = NULL;
	const char* print_class_name = "";

	printf("===============================================================================\n");
	printf("%-16.16s | test case\n", "class");
	printf("-------------------------------------------------------------------------------\n");

	etest_map_node_t* it = etest_map_begin(&g_test_ctx.info.case_table);
	for (; it != NULL; it = etest_map_next(&g_test_ctx.info.case_table, it))
	{
		etest_case_t* case_data = CONTAINER_OF(it, etest_case_t, node.table);
		if (last_class_name != case_data->data.class_name)
		{
			last_class_name = case_data->data.class_name;
			print_class_name = last_class_name;
			c_class++;
		}

		printf("%-16.16s | %s\n", print_class_name, case_data->data.case_name);
		print_class_name = "";

		c_test++;
	}

	printf("-------------------------------------------------------------------------------\n");
	printf("total: %u class%s, %u test%s\n",
		c_class, c_class > 1 ? "es" : "",
		c_test, c_test > 1 ? "s" : "");
	printf("===============================================================================\n");
}

static void _etest_setup_arg_repeat(const char* str)
{
	int repeat = 1;
	sscanf(str, "%d", &repeat);
	g_test_ctx.counter.repeat.repeat = (unsigned)repeat;
}

static void _etest_setup_arg_print_time(const char* str)
{
	int val = 1;
	sscanf(str, "%d", &val);
	g_test_ctx.mask.print_time = !!val;
}

static void _etest_setup_arg_random_seed(const char* str)
{
	long long val = time(NULL);
	sscanf(str, "%lld", &val);

	_test_srand(val);
}

static int _test_setup_args(int argc, char* argv[])
{
	enum test_opt
	{
		etest_list_tests = 1,
		etest_filter,
		etest_also_run_disabled_tests,
		etest_repeat,
		etest_shuffle,
		etest_random_seed,
		etest_print_time,
		etest_break_on_failure,
		help,
	};

	test_optparse_long_opt_t longopts[] = {
		{ "etest_list_tests",				etest_list_tests,				OPTPARSE_OPTIONAL },
		{ "etest_filter",					etest_filter,					OPTPARSE_OPTIONAL },
		{ "etest_also_run_disabled_tests",	etest_also_run_disabled_tests,	OPTPARSE_OPTIONAL },
		{ "etest_repeat",					etest_repeat,					OPTPARSE_OPTIONAL },
		{ "etest_shuffle",					etest_shuffle,					OPTPARSE_OPTIONAL },
		{ "etest_random_seed",				etest_random_seed,				OPTPARSE_OPTIONAL },
		{ "etest_print_time",				etest_print_time,				OPTPARSE_OPTIONAL },
		{ "test_break_on_failure",			etest_break_on_failure,			OPTPARSE_OPTIONAL },
		{ "help",							help,							OPTPARSE_OPTIONAL },
		{ 0,								0,								OPTPARSE_NONE },
	};

	test_optparse_t options;
	test_optparse_init(&options, argv);

	int option;
	while ((option = test_optparse_long(&options, longopts, NULL)) != -1) {
		switch (option) {
		case etest_list_tests:
			_etest_list_tests();
			return -1;
		case etest_filter:
			_etest_setup_arg_pattern(options.optarg);
			break;
		case etest_also_run_disabled_tests:
			g_test_ctx.mask.also_run_disabled_tests = 1;
			break;
		case etest_repeat:
			_etest_setup_arg_repeat(options.optarg);
			break;
		case etest_shuffle:
			g_test_ctx.mask.shuffle = 1;
			break;
		case etest_random_seed:
			_etest_setup_arg_random_seed(options.optarg);
			break;
		case etest_print_time:
			_etest_setup_arg_print_time(options.optarg);
			break;
		case etest_break_on_failure:
			g_test_ctx.mask.break_on_failure = 1;
			break;
		case help:
			printf(
				"This program contains tests written using Test. You can use the\n"
				"following command line flags to control its behavior:\n"
				"\n"
				"Test Selection:\n"
				"  "COLOR_GREEN("--etest_list_tests")"\n"
				"      List the names of all tests instead of running them. The name of\n"
				"      TEST(Foo, Bar) is \"Foo.Bar\".\n"
				"  "COLOR_GREEN("--etest_filter=") COLOR_YELLO("POSTIVE_PATTERNS[") COLOR_GREEN("-") COLOR_YELLO("NEGATIVE_PATTERNS]")"\n"
				"      Run only the tests whose name matches one of the positive patterns but\n"
				"      none of the negative patterns. '?' matches any single character; '*'\n"
				"      matches any substring; ':' separates two patterns.\n"
				"  "COLOR_GREEN("--etest_also_run_disabled_tests")"\n"
				"      Run all disabled tests too.\n"
				"\n"
				"Test Execution:\n"
				"  "COLOR_GREEN("--etest_repeat=")COLOR_YELLO("[COUNT]")"\n"
				"      Run the tests repeatedly; use a negative count to repeat forever.\n"
				"  "COLOR_GREEN("--etest_shuffle")"\n"
				"      Randomize tests' orders on every iteration.\n"
				"  "COLOR_GREEN("--etest_random_seed=") COLOR_YELLO("[NUMBER]") "\n"
				"      Random number seed to use for shuffling test orders (between 0 and\n"
				"      99999. By default a seed based on the current time is used for shuffle).\n"
				"\n"
				"Test Output:\n"
				"  "COLOR_GREEN("--etest_print_time=") COLOR_YELLO("(") COLOR_GREEN("0") COLOR_YELLO("|") COLOR_GREEN("1") COLOR_YELLO(")") "\n"
				"      Don't print the elapsed time of each test.\n"
				"\n"
				"Assertion Behavior:\n"
				"  "COLOR_GREEN("--etest_break_on_failure")"\n"
				"      Turn assertion failures into debugger break-points.\n"
				);
			return -1;
		default:
			break;
		}
	}

	return 0;
}

void etest_register_case(etest_case_t* data)
{
	if (etest_map_insert(&g_test_ctx.info.case_table, &data->node.table) < 0)
	{
		return;
	}
	_test_list_push_back(&g_test_ctx.info.case_list, &data->node.queue);
}

static void _test_run_test_loop(void)
{
	_test_reset_all_test();
	printf(COLOR_GREEN("[==========]") " total %u test%s registered.\n",
		g_test_ctx.info.case_list.size,
		g_test_ctx.info.case_list.size > 1 ? "s" : "");

	etest_timestamp_get(&g_test_ctx.timestamp.tv_total_start);

	g_test_ctx.runtime.cur_it = _test_list_begin(&g_test_ctx.info.case_list);
	for (; g_test_ctx.runtime.cur_it != NULL;
		g_test_ctx.runtime.cur_it = _test_list_next(&g_test_ctx.info.case_list, g_test_ctx.runtime.cur_it))
	{
		g_test_ctx.runtime.cur_case = CONTAINER_OF(g_test_ctx.runtime.cur_it, etest_case_t, node.queue);
		_test_run_case();
	}

	etest_timestamp_get(&g_test_ctx.timestamp.tv_total_end);

	_test_show_report();
}

/**
* 测试用例随机排序
*/
static void _test_shuffle_cases(void)
{
	test_list_t copy_case_list = TEST_LIST_INITIALIZER;

	while (_test_list_size(&g_test_ctx.info.case_list) != 0)
	{
		unsigned idx = _test_rand() % _test_list_size(&g_test_ctx.info.case_list);

		unsigned i = 0;
		etest_list_node_t* it = _test_list_begin(&g_test_ctx.info.case_list);
		for (; i < idx; i++, it = _test_list_next(&g_test_ctx.info.case_list, it));

		_test_list_erase(&g_test_ctx.info.case_list, it);
		_test_list_push_back(&copy_case_list, it);
	}

	g_test_ctx.info.case_list = copy_case_list;
}

int etest_run_tests(int argc, char* argv[])
{
	/* 初始化随机数 */
	_test_srand(time(NULL));

	/* 解析参数 */
	if (_test_setup_args(argc, argv) < 0)
	{
		goto fin;
	}

	/* 必要时对测试用例重排序 */
	if (g_test_ctx.mask.shuffle)
	{
		_test_shuffle_cases();
	}

	for (g_test_ctx.counter.repeat.repeated = 0;
		g_test_ctx.counter.repeat.repeated < g_test_ctx.counter.repeat.repeat;
		g_test_ctx.counter.repeat.repeated++)
	{
		if (g_test_ctx.counter.repeat.repeat > 1)
		{
			printf(COLOR_YELLO("[==========]") " start loop: %u/%u\n",
				g_test_ctx.counter.repeat.repeated + 1, g_test_ctx.counter.repeat.repeat);
		}

		_test_run_test_loop();

		if (g_test_ctx.counter.repeat.repeat > 1)
		{
			printf(COLOR_YELLO("[==========]") " end loop (%u/%u)\n",
				g_test_ctx.counter.repeat.repeated + 1, g_test_ctx.counter.repeat.repeat);
			if (g_test_ctx.counter.repeat.repeated < g_test_ctx.counter.repeat.repeat - 1)
			{
				printf("\n");
			}
		}
	}

	/* 清理资源 */
fin:
	if (g_test_ctx.filter.postive_patterns != NULL)
	{
		free(g_test_ctx.filter.postive_patterns);
		g_test_ctx.filter.postive_patterns = NULL;
		g_test_ctx.filter.negative_patterns = NULL;
		g_test_ctx.filter.n_postive = 0;
		g_test_ctx.filter.n_negative = 0;
	}

	return (int)g_test_ctx.counter.result.failed;
}

void etest_assert_fail(const char *expr, const char *file, int line, const char *func)
{
	fprintf(stderr, "Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
	fflush(NULL);
	abort();
}

void etest_assert_num_eq(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a == b, "==", "%lld", s_a, a, s_b, b, file, line);
}

void etest_assert_num_ne(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a != b, "!=", "%lld", s_a, a, s_b, b, file, line);
}

void etest_assert_num_lt(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a < b, "<", "%lld", s_a, a, s_b, b, file, line);
}

void etest_assert_num_le(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a <= b, "<=", "%lld", s_a, a, s_b, b, file, line);
}

void etest_assert_num_gt(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a > b, ">", "%lld", s_a, a, s_b, b, file, line);
}

void etest_assert_num_ge(long long a, long long b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a >= b, ">=", "%lld", s_a, a, s_b, b, file, line);
}

void etest_assert_ptr_eq(void* a, void* b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a == b, "==", "%p", s_a, a, s_b, b, file, line);
}

void etest_assert_ptr_ne(void* a, void* b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a != b, "!=", "%p", s_a, a, s_b, b, file, line);
}

void etest_assert_str_eq(const char* a, const char* b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(strcmp(a, b) == 0, "!=", "%p", s_a, a, s_b, b, file, line);
}

void etest_assert_str_ne(const char* a, const char* b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(strcmp(a, b) != 0, "!=", "%p", s_a, a, s_b, b, file, line);
}

void etest_assert_flt_eq(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a == b, "==", "%f", s_a, a, s_b, b, file, line);
}

void etest_assert_flt_ne(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a != b, "!=", "%f", s_a, a, s_b, b, file, line);
}

void etest_assert_flt_lt(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a < b, "<", "%f", s_a, a, s_b, b, file, line);
}

void etest_assert_flt_le(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a <= b, "<=", "%f", s_a, a, s_b, b, file, line);
}

void etest_assert_flt_gt(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a > b, ">", "%f", s_a, a, s_b, b, file, line);
}

void etest_assert_flt_ge(double a, double b, const char* s_a, const char* s_b, const char* file, int line)
{
	JUDGE_GENERIC_TEMPLATE(a >= b, ">=", "%f", s_a, a, s_b, b, file, line);
}

/************************************************************************/
/* LOG                                                                  */
/************************************************************************/

const char* etest_pretty_file(const char* file)
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
