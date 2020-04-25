#ifndef __EAF_POWERPACK_INTERNAL_H__
#define __EAF_POWERPACK_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/powerpack.h"
#include "uv.h"

uv_loop_t* powerpack_get_uv(void);
void powerpack_notify(void);

uint32_t powerpack_get_service_id(void);

#ifdef __cplusplus
}
#endif
#endif
