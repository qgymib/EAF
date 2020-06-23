#ifndef __EAF_POWERPACK_INTERNAL_H__
#define __EAF_POWERPACK_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/powerpack.h"
#include "uv.h"

/**
 * @brief Get uv_loop
 * @return		uv_loop_t
 */
uv_loop_t* eaf_uv_get(void);

/**
 * @brief Notify that uv_loop has modified
 */
void eaf_uv_mod(void);

/**
 * @brief Get PowerPack service id
 * @warning PowerPack is a shared service, so make sure all the operation is non-block
 * @return		Service ID
 */
uint32_t powerpack_get_service_id(void);

#ifdef __cplusplus
}
#endif
#endif
