/** @file
 * Macros
 */
#ifndef __EAF_UTILS_DEFINE_H__
#define __EAF_UTILS_DEFINE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def EAF_API
 * @brief API describe
 */
#if defined(_WIN32)
#	if defined(EAF_SHARED_BUILDING)
		/* Building shared library. */
#		define EAF_API	__declspec(dllexport)
#	elif defined(EAF_SHARED_USING)
		/* Using shared library. */
#		define EAF_API	__declspec(dllimport)
#	else
		/* Building static library. */
#		define EAF_API	/* nothing */
#	endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define EAF_API	__attribute__((visibility("default")))
#else
#	define EAF_API	/* nothing */
#endif

/**
 * @brief Get struct address according to member address
 * @param ptr		Member address
 * @param TYPE		Struct type
 * @param member	Member name
 * @return			Struct address
 */
#define EAF_CONTAINER_OF(ptr, TYPE, member)	\
	((TYPE*)((char*)(ptr) - (size_t)&((TYPE*)0)->member))

/**
 * @brief Force access data
 * @param TYPE	Data type
 * @param x		data
 * @return		data value
 */
#define EAF_ACCESS(TYPE, x)		(*(volatile TYPE*)&(x))

/**
 * @brief Get array length
 * @param arr	Array
 * @return		Length
 */
#define EAF_ARRAY_SIZE(arr)		(sizeof(arr) / sizeof(arr[0]))

/**
 * @brief Align `size` to `align`
 * @param size	The number to be aligned
 * @param align	Alignment
 * @return		The number that aligned
 */
#define EAF_ALIGN(size, align)	\
	(((uintptr_t)(size) + ((uintptr_t)(align) - 1)) & ~((uintptr_t)(align) - 1))

/**
 * @brief Expand content
 * @param[in] x	Code to expand
 */
#define EAF_EXPAND(x)			x

/**
 * @brief Join `a' and `b' as one token
 * @param[in] a	Token `a'
 * @param[in] b	Token `b'
 */
#define EAF_JOIN(a, b)			EAF_INTERNAL_JOIN(a, b)
#define EAF_INTERNAL_JOIN(a, b)	a##b

/**
 * @def EAF_COUNT_ARG
 * @brief Count the number of arguments in macro
 */
#ifdef _MSC_VER // Microsoft compilers
#   define EAF_COUNT_ARG(...)  EAF_INTERNAL_EXPAND_ARGS_PRIVATE(EAF_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#   define EAF_INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define EAF_INTERNAL_EXPAND_ARGS_PRIVATE(...) EAF_EXPAND(EAF_INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define EAF_INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#else // Non-Microsoft compilers
#   define EAF_COUNT_ARG(...) EAF_INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define EAF_INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#endif

/**
 * @brief Suppress unused variable warning
 * @param[in] ...	argument list
 */
#define EAF_SUPPRESS_UNUSED_VARIABLE(...)	\
	EAF_EXPAND(EAF_JOIN(EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_, EAF_COUNT_ARG(__VA_ARGS__))(__VA_ARGS__))
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_1(_0)	\
	(void)_0
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_2(_0, _1)	\
	(void)_0; (void)_1
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_3(_0, _1, _2)	\
	(void)_0; (void)_1; (void)_2
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_4(_0, _1, _2, _3)	\
	(void)_0; (void)_1; (void)_2; (void)_3
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_5(_0, _1, _2, _3, _4)	\
	(void)_0; (void)_1; (void)_2; (void)_3; (void)_4
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_6(_0, _1, _2, _3, _4, _5)	\
	(void)_0; (void)_1; (void)_2; (void)_3; (void)_4; (void)_5
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_7(_0, _1, _2, _3, _4, _5, _6)	\
	(void)_0; (void)_1; (void)_2; (void)_3; (void)_4; (void)_5; (void)_6
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_8(_0, _1, _2, _3, _4, _5, _6, _7)	\
	(void)_0; (void)_1; (void)_2; (void)_3; (void)_4; (void)_5; (void)_6; (void)_7
#define EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_9(_0, _1, _2, _3, _4, _5, _6, _7, _8)	\
	(void)_0; (void)_1; (void)_2; (void)_3; (void)_4; (void)_5; (void)_6; (void)_7; (void)_8

/**
 * @internal
 * Define a set of integral type aliases with specific width requirements,
 * along with macros specifying their limits and macro functions to create
 * values of these types.
 * @note define `EAF_NO_STDINT_MSVC_2008' to disable stdint support for
 * MSVC-2008
 */
#if defined(_MSC_VER) && _MSC_VER < 1600 && !defined(EAF_NO_STDINT_MSVC_2008)
	/* copy from libuv-1.37.0 */
#	include "EAF/utils/stdint-msvc2008.h"
#else
#	include <stdint.h>
#endif

/**
 * @internal
 * SAL provides a set of annotations to describe how a function uses
 * its parameters - the assumptions it makes about them, and the guarantees it
 * makes upon finishing.
 */
#if defined(_MSC_VER)
#include <sal.h>
#else

/**
 * @brief Input parameter.
 * 
 * Annotates input parameters that are scalars, structures, pointers to
 * structures and the like. Explicitly may be used on simple scalars. The
 * parameter must be valid in pre-state and will not be modified.
 */
#define _In_

/**
 * @brief Like #_In_, but parameter may be null
 * @see \_In\_
 */
#define _In_opt_

/**
 * @brief Output parameter.
 *
 * Annotates output parameters that are scalars, structures, pointers to
 * structures and the like. Don't apply this annotation to an object that
 * can't return a value¡ªfor example, a scalar that's passed by value. The
 * parameter doesn't have to be valid in pre-state but must be valid in
 * post-state.
 */
#define _Out_

/**
 * @brief Like #_Out_, but parameter may be optional.
 * @see \_Out\_
 */
#define _Out_opt_

/**
 * @brief Parameter can't be null, and in the post-state the pointed-to location can't
 *   be null and must be valid.
 */
#define _Outptr_

/**
 * @brief Parameter may be null, but in the post-state the pointed-to location can't
 *   be null and must be valid.
 */
#define _Outptr_opt_

/**
 * @brief Parameter may be null, and in the post-state the pointed-to location can be
 *   null.
 */
#define _Outptr_opt_result_maybenull_

/**
 * @brief Annotates a parameter that will be changed by the function. It must be valid
 *   in both pre-state and post-state, but is assumed to have different values
 * before and after the call. Must apply to a modifiable value.
 */
#define _Inout_

/**
 * @brief Like #_Inout_, but parameters are optional.
 * @see \_Inout\_
 */
#define _Inout_opt_

/**
 * @brief Annotates a parameter that will be invalid when the function returns.
 */
#define _Post_invalid_

/**
 * @brief Indicates that the parameter is a format string for use in a printf expression.
 */
#define _Printf_format_string_

/**
 * @brief Indicates that the parameter is a format string for use in a scanf expression.
 */
#define _Scanf_format_string_

#endif

#ifdef __cplusplus
}
#endif
#endif
