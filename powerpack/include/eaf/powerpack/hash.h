/** @file
 * Hash algorithm
 */
#ifndef __EAF_POWERPACK_HASH_H__
#define __EAF_POWERPACK_HASH_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-Hash Hash
 *
 * Here is a example for how to use hash functions
 *
 * @code
 void example(void)
 {
     // the data
     const char* str = "hello world";
     // calculate hash
     uint32_t hval = eaf_hash32_bkdr(str, strlen(str), 0);
     // print hash result
     printf("hval: %"PRIx32"\n", hval);
 }
 * @endcode
 *
 * @{
 */

#include "eaf/eaf.h"

/**
 * @brief Make 32 bits hash with BKDR algorithm
 * @param[in] data	The data
 * @param[in] size	Data size in byte
 * @param[in] seed	Initialize seed
 * @return			Hash result in 32 bits
 */
uint32_t eaf_hash32_bkdr(_In_ const void* data, _In_ size_t size,
	_In_ uint32_t seed)
	EAF_ATTRIBUTE_ACCESS(read_only, 1, 2)
	EAF_ATTRIBUTE_PURE;

/**
 * @brief Make 64 bits hash with BKDR algorithm
 * @param[in] data	The data
 * @param[in] size	Data size in byte
 * @param[in] seed	Initialize seed
 * @return			Hash result in 64 bits
 */
uint64_t eaf_hash64_bkdr(_In_ const void* data, _In_ size_t size,
	_In_ uint64_t seed)
	EAF_ATTRIBUTE_ACCESS(read_only, 1, 2)
	EAF_ATTRIBUTE_PURE;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
