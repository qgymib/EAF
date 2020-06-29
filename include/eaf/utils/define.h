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
 * @defgroup SAL SAL
 * @{
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
 * @brief Like #_Out_, but parameters are optional.
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

#endif

/**
 * Group: SAL
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
