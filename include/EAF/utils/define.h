#ifndef __EAF_UTILS_DEFINE_H__
#define __EAF_UTILS_DEFINE_H__
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#	define EAF_NORETURN		__declspec(noreturn)
#elif defined(__clang__)
#	define EAF_NORETURN		_Noreturn
#elif defined(__GNUC__)
#	define EAF_NORETURN		__attribute__((__noreturn__))
#else
#	define EAF_NORETURN
#endif

/**
* 根据结构体成员变量地址找到结构体首地址
* @param ptr	地址
* @param TYPE	结构体类型
* @param member	地址对应成员
* @return		结构体地址
*/
#define EAF_CONTAINER_OF(ptr, TYPE, member)	\
	((TYPE*)((char*)(ptr) - (size_t)&((TYPE*)0)->member))

/**
* 强制访问数据
* @param TYPE	数据类型
* @param x		数据
* @return		数据值
*/
#define EAF_ACCESS(TYPE, x)		(*(volatile TYPE*)&(x))

/**
* 获取数组长度
* @param arr	数组
* @return		长度
*/
#define EAF_ARRAY_SIZE(arr)		(sizeof(arr) / sizeof(arr[0]))

/**
* 将size对齐至algin的整数倍
* @param size	被对齐数字
* @param align	对齐倍率
* @return		对齐之后的数据
*/
#define EAF_ALIGN(size, align)	\
	(((uintptr_t)(size) + ((uintptr_t)(align) - 1)) & ~((uintptr_t)(align) - 1))

#ifdef __cplusplus
}
#endif
#endif
