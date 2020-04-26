/** @file
 * Hash algorithm
 */
#ifndef __EAF_POWERPACK_HASH_H__
#define __EAF_POWERPACK_HASH_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "eaf/eaf.h"

/**
 * @brief Make 32 bits hash with BKDR algorithm
 * @param[in] data	The data
 * @param[in] size	Data size in byte
 * @param[in] seed	Initialize seed
 * @return			Hash result in 32 bits
 */
uint32_t eaf_hash32_bkdr(_In_ const void* data, _In_ size_t size,
	_In_ uint32_t seed);

#ifdef __cplusplus
}
#endif
#endif
