/**
 * @file
 */
#ifndef __EAF_UTILS_DEFINE_H__
#define __EAF_UTILS_DEFINE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Tool
 * Some common macros to help program.
 */
/**@{*/

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
#		define EAF_API
#	endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define EAF_API	__attribute__((visibility("default")))
#else
#	define EAF_API
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
/**@cond DOXYGEN_INTERNAL*/
#define EAF_INTERNAL_JOIN(a, b)	a##b
/**@endcond*/

/**
 * @def EAF_COUNT_ARG
 * @brief Count the number of arguments in macro
 */
#ifdef _MSC_VER // Microsoft compilers
#   define EAF_COUNT_ARG(...)  EAF_INTERNAL_EXPAND_ARGS_PRIVATE(EAF_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
/**@cond DOXYGEN_INTERNAL*/
#   define EAF_INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define EAF_INTERNAL_EXPAND_ARGS_PRIVATE(...) EAF_EXPAND(EAF_INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define EAF_INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
/**@endcond*/
#else // Non-Microsoft compilers
#   define EAF_COUNT_ARG(...) EAF_INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
/**@cond DOXYGEN_INTERNAL*/
#   define EAF_INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
/**@endcond*/
#endif

/**
 * @brief Suppress unused variable warning
 * @param[in] ...	argument list
 */
#define EAF_SUPPRESS_UNUSED_VARIABLE(...)	\
	EAF_EXPAND(EAF_JOIN(EAF_INTERNAL_SUPPRESS_UNUSED_VARIABLE_, EAF_COUNT_ARG(__VA_ARGS__))(__VA_ARGS__))
/**@cond DOXYGEN_INTERNAL*/
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
/**@endcond*/

/**
 * @brief Repeat `v' for `n' times
 * Current support 0 <= n <= 31
 * @param[in] n	How many times need to repeat
 * @param[in] v	The value to be repeat
 */
#define EAF_REPEAT(n, v)   \
	EAF_JOIN(EAF_INTERNAL_REPEAT_, n)(v)
/**@cond DOXYGEN_INTERNAL*/
#define EAF_INTERNAL_REPEAT_0(v)
#define EAF_INTERNAL_REPEAT_1(v)	v
#define EAF_INTERNAL_REPEAT_2(v)	EAF_INTERNAL_REPEAT_1(v), v
#define EAF_INTERNAL_REPEAT_3(v)	EAF_INTERNAL_REPEAT_2(v), v
#define EAF_INTERNAL_REPEAT_4(v)	EAF_INTERNAL_REPEAT_3(v), v
#define EAF_INTERNAL_REPEAT_5(v)	EAF_INTERNAL_REPEAT_4(v), v
#define EAF_INTERNAL_REPEAT_6(v)	EAF_INTERNAL_REPEAT_5(v), v
#define EAF_INTERNAL_REPEAT_7(v)	EAF_INTERNAL_REPEAT_6(v), v
#define EAF_INTERNAL_REPEAT_8(v)	EAF_INTERNAL_REPEAT_7(v), v
#define EAF_INTERNAL_REPEAT_9(v)	EAF_INTERNAL_REPEAT_8(v), v
#define EAF_INTERNAL_REPEAT_10(v)	EAF_INTERNAL_REPEAT_9(v), v
#define EAF_INTERNAL_REPEAT_11(v)	EAF_INTERNAL_REPEAT_10(v), v
#define EAF_INTERNAL_REPEAT_12(v)	EAF_INTERNAL_REPEAT_11(v), v
#define EAF_INTERNAL_REPEAT_13(v)	EAF_INTERNAL_REPEAT_12(v), v
#define EAF_INTERNAL_REPEAT_14(v)	EAF_INTERNAL_REPEAT_13(v), v
#define EAF_INTERNAL_REPEAT_15(v)	EAF_INTERNAL_REPEAT_14(v), v
#define EAF_INTERNAL_REPEAT_16(v)	EAF_INTERNAL_REPEAT_15(v), v
#define EAF_INTERNAL_REPEAT_17(v)	EAF_INTERNAL_REPEAT_16(v), v
#define EAF_INTERNAL_REPEAT_18(v)	EAF_INTERNAL_REPEAT_17(v), v
#define EAF_INTERNAL_REPEAT_19(v)	EAF_INTERNAL_REPEAT_18(v), v
#define EAF_INTERNAL_REPEAT_20(v)	EAF_INTERNAL_REPEAT_19(v), v
#define EAF_INTERNAL_REPEAT_21(v)	EAF_INTERNAL_REPEAT_20(v), v
#define EAF_INTERNAL_REPEAT_22(v)	EAF_INTERNAL_REPEAT_21(v), v
#define EAF_INTERNAL_REPEAT_23(v)	EAF_INTERNAL_REPEAT_22(v), v
#define EAF_INTERNAL_REPEAT_24(v)	EAF_INTERNAL_REPEAT_23(v), v
#define EAF_INTERNAL_REPEAT_25(v)	EAF_INTERNAL_REPEAT_24(v), v
#define EAF_INTERNAL_REPEAT_26(v)	EAF_INTERNAL_REPEAT_25(v), v
#define EAF_INTERNAL_REPEAT_27(v)	EAF_INTERNAL_REPEAT_26(v), v
#define EAF_INTERNAL_REPEAT_28(v)	EAF_INTERNAL_REPEAT_27(v), v
#define EAF_INTERNAL_REPEAT_29(v)	EAF_INTERNAL_REPEAT_28(v), v
#define EAF_INTERNAL_REPEAT_30(v)	EAF_INTERNAL_REPEAT_29(v), v
#define EAF_INTERNAL_REPEAT_31(v)	EAF_INTERNAL_REPEAT_30(v), v
/**@endcond*/

/**@}*/

/**
 * @internal
 * Define a set of integral type aliases with specific width requirements,
 * along with macros specifying their limits and macro functions to create
 * values of these types.
 * @note define `EAF_NO_STDINT_MSVC_2008' to disable stdint support for
 *   MSVC-2008
 */
#if defined(_MSC_VER) && _MSC_VER < 1600 && !defined(EAF_NO_STDINT_MSVC_2008)
	/* copy from libuv-1.37.0 */
#	include "EAF/utils/stdint-msvc2008.h"
#else
#	include <stdint.h>
#endif
#include <stddef.h>

/**
 * @name SAL
 * SAL provides a set of annotations to describe how a function uses
 * its parameters - the assumptions it makes about them, and the guarantees it
 * makes upon finishing.
 */
/**@{*/
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

/**@}*/

/**
 * @name Attribute
 * Use attributes to specify certain function properties that:
 * + Static analysis - better warnings and errors help you catch errors before they become a real issue.
 * + Optimizations - compiler hints help speed up your code.
 * + Manage public APIs
 *   + Visibility - keeping internal symbols private can make your program faster and smaller.
 *   + Versioning - help consumers avoid functions which are deprecated or too new for all the platforms they want to support.
 * + C/C++ interoperability - make it easier to use code in both C and C++ compilers.
 */
/**@{*/

/**@cond DOXYGEN_INTERNAL*/
/**
 * @def EAF_GNUC_PREREQ
 * @brief Convenience macro to test the version of gcc.
 *
 * Use like this:
 * ```
 * #if EAF_GNUC_PREREQ (2,8)
 * ... code requiring gcc 2.8 or later ...
 * #endif
 * ```
 * @note only works for GCC 2.0 and later, because \__GNUC_MINOR\__ was added in 2.0.
 */
#if defined __GNUC__ && defined __GNUC_MINOR__
#	define EAF_GNUC_PREREQ(maj, min) \
		((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#	define EAF_GNUC_PREREQ(maj, min) 0
#endif

/**
 * @def EAF_HAS_ATTRIBUTE
 * @brief Test whether the `attribute` referenced by its `operand` is
 *   recognized by compiler
 */
#if defined(__has_attribute)
#	define EAF_HAS_ATTRIBUTE(attribute) __has_attribute(attribute)
#else
#	define EAF_HAS_ATTRIBUTE(attribute) (0)
#endif
/**@endcond*/

/**
 * @def EAF_ATTRIBUTE_ACCESS
 * @brief The access attribute enables the detection of invalid or unsafe accesses
 *   by functions to which they apply or their callers, as well as write-only
 *   accesses to objects that are never read from.
 *
 * + access (access-mode, ref-index)
 * + access (access-mode, ref-index, size-index)
 *
 * The access attribute specifies that a function to whose by-reference arguments
 * the attribute applies accesses the referenced object according to access-mode.
 * The access-mode argument is required and must be one of four names: read_only,
 * read_write, write_only, or none. The remaining two are positional arguments.
 *
 * The required ref-index positional argument denotes a function argument of
 * pointer (or in C++, reference) type that is subject to the access. The same
 * pointer argument can be referenced by at most one distinct access attribute.
 *
 * The optional size-index positional argument denotes a function argument of
 * integer type that specifies the maximum size of the access. The size is the
 * number of elements of the type referenced by ref-index, or the number of
 * bytes when the pointer type is void*. When no size-index argument is specified,
 * the pointer argument must be either null or point to a space that is suitably
 * aligned and large for at least one object of the referenced type (this implies
 * that a past-the-end pointer is not a valid argument). The actual size of the
 * access may be less but it must not be more.
 *
 * The *read_only* access mode specifies that the pointer to which it applies is
 * used to read the referenced object but not write to it. Unless the argument
 * specifying the size of the access denoted by size-index is zero, the
 * referenced object must be initialized. The mode implies a stronger guarantee
 * than the const qualifier which, when cast away from a pointer, does not
 * prevent the pointed-to object from being modified. Examples of the use of
 * the read_only access mode is the argument to the puts function, or the second
 * and third arguments to the memcpy function.
 *
 * The *read_write* access mode applies to arguments of pointer types without the
 * const qualifier. It specifies that the pointer to which it applies is used
 * to both read and write the referenced object. Unless the argument specifying
 * the size of the access denoted by size-index is zero, the object referenced
 * by the pointer must be initialized. An example of the use of the read_write
 * access mode is the first argument to the strcat function.
 *
 * The *write_only* access mode applies to arguments of pointer types without the
 * const qualifier. It specifies that the pointer to which it applies is used
 * to write to the referenced object but not read from it. The object referenced
 * by the pointer need not be initialized. An example of the use of the
 * write_only access mode is the first argument to the strcpy function, or the
 * first two arguments to the fgets function.
 *
 * The access mode *none* specifies that the pointer to which it applies is not
 * used to access the referenced object at all. Unless the pointer is null the
 * pointed-to object must exist and have at least the size as denoted by the
 * size-index argument. The object need not be initialized. The mode is
 * intended to be used as a means to help validate the expected object size,
 * for example in functions that call __builtin_object_size.
 */
#if EAF_GNUC_PREREQ(10, 0)
#	define EAF_ATTRIBUTE_ACCESS(access_mode, ref_index, ...) \
		__attribute__((__access__(access_mode, ref_index, ##__VA_ARGS__)))
#else
#	define EAF_ATTRIBUTE_ACCESS(access_mode, ref_index, ...)
#endif

/**
 * @def EAF_ATTRIBUTE_FORMAT_PRINTF
 * @brief The format attribute specifies that a function takes printf style
 *   arguments that should be type-checked against a format string.
 */
#if EAF_GNUC_PREREQ(3, 1)
#	define EAF_ATTRIBUTE_FORMAT_PRINTF(string_index, first_to_check) \
		__attribute__((__format__(__printf__, string_index, first_to_check)))
#else
#	define EAF_ATTRIBUTE_FORMAT_PRINTF(string_index, first_to_check)
#endif

/**
 * @def EAF_ATTRIBUTE_NONNULL
 * @brief The nonnull attribute may be applied to a function that takes at
 *   least one argument of a pointer type.
 *
 *  It indicates that the referenced arguments must be non-null pointers.
 */
#if EAF_GNUC_PREREQ(3, 3)
#	define EAF_ATTRIBUTE_NONNULL(...)	__attribute__((__nonnull__(__VA_ARGS__)))
#else
#	define EAF_ATTRIBUTE_NONNULL(...)
#endif

/**
 * @def EAF_ATTRIBUTE_NOTHROW
 * @brief The nothrow attribute is used to inform the compiler that a function
 *   cannot throw an exception.
 */
#if EAF_GNUC_PREREQ(3, 3)
#	define EAF_ATTRIBUTE_NOTHROW	__attribute__((__nothrow__))
#else
#	define EAF_ATTRIBUTE_NOTHROW
#endif

/**
 * @def EAF_ATTRIBUTE_PURE
 * @brief Calls to functions that have no observable effects on the state of
 *   the program other than to return a value may lend themselves to optimizations
 *   such as common subexpression elimination. Declaring such functions with
 *   the pure attribute allows GCC to avoid emitting some calls in repeated
 *   invocations of the function with the same argument values.
 *
 * The pure attribute prohibits a function from modifying the state of the
 * program that is observable by means other than inspecting the function's
 * return value. However, functions declared with the pure attribute can safely
 * read any non-volatile objects, and modify the value of objects in a way that
 * does not affect their return value or the observable state of the program. 
 */
#if EAF_GNUC_PREREQ(2, 96)
#	define EAF_ATTRIBUTE_PURE	__attribute__((__pure__))
#else
#	define EAF_ATTRIBUTE_PURE
#endif

/**@}*/

#ifdef __cplusplus
}
#endif
#endif
