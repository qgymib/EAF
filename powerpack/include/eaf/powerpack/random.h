/**
 * @file
 * Random utils.
 */
#ifndef __EAF_POWERPACK_RANDOM_H__
#define __EAF_POWERPACK_RANDOM_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup PowerPack
 * @defgroup PowerPack-Random Random
 * @{
 */

#include "eaf/eaf.h"

/**
 * @brief Initialize random
 *
 * `Random' always try to use hardware random generator to generate true
 * random data. In case of hardware random device failure, we need a seed for
 * software pseudo-random algorithm.
 *
 * @param[in] seed	The seed for pseudo-random algorithm
 * @return			#eaf_errno
 */
EAF_API int eaf_random_init(_In_ uint32_t seed);

/**
 * @brief Exit random
 */
EAF_API void eaf_random_exit(void);

/**
 * @brief Fill buffer with random data
 * @param[out] buffer	Buffer area
 * @param[in] size		Buffer size
 */
EAF_API void eaf_random(_Out_ void* buffer, _In_ size_t size)
	EAF_ATTRIBUTE_ACCESS(write_only, 1, 2)
	EAF_ATTRIBUTE_NONNULL(1);

/**
 * @brief Generate a random uint32_t integer
 * @return		Random integer
 */
EAF_API uint32_t eaf_random32(void);

/**
 * @brief Generate a random uinit64_t integer
 * @return		Random integer
 */
EAF_API uint64_t eaf_random64(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
