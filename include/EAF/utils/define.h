/** @file
 * Macros
 */
#ifndef __EAF_UTILS_DEFINE_H__
#define __EAF_UTILS_DEFINE_H__
#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}
#endif
#endif
