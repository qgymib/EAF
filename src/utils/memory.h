#ifndef __EAF_UTILS_MEMORY_INTERNAL_H__
#define __EAF_UTILS_MEMORY_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#if !defined(EAF_MALLOC_FN)
#	define EAF_MALLOC_FN			malloc
#else
extern void* EAF_MALLOC_FN(size_t size);
#endif
#define EAF_MALLOC(size)			EAF_MALLOC_FN(size)


#if !defined(EAF_CALLOC_FN)
#	define EAF_CALLOC_FN			calloc
#else
extern void* EAF_CALLOC_FN(size_t nmemb, size_t size);
#endif
#define EAF_CALLOC(nmemb, size)		EAF_CALLOC_FN(nmemb, size)


#if !defined(EAF_REALLOC_FN)
#	define EAF_REALLOC_FN			realloc
#else
extern void* EAF_REALLOC_FN(void* ptr, size_t size);
#endif
#define EAF_REALLOC(ptr, size)		EAF_REALLOC_FN(ptr, size)


#if !defined(EAF_FREE_FN)
#	define EAF_FREE_FN				free
#else
extern void EAF_FREE_FN(void* ptr);
#endif
#define EAF_FREE(ptr)				EAF_FREE_FN(ptr)

#ifdef __cplusplus
}
#endif
#endif	/* __EAF_UTILS_MEMORY_INTERNAL_H__ */
