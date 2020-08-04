#ifndef __EAF_POWERPACK_LOG_INTERNAL_H__
#define __EAF_POWERPACK_LOG_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/powerpack/log.h"

typedef struct eaf_log_ctx
{
	eaf_log_level_t				filter_level;	/**< Filter level */

	struct
	{
		eaf_log_callback_fn		fn;				/**< User callback */
		void*					arg;			/**< User defined argument */
	}cb;
}eaf_log_ctx_t;

/**
 * @brief Initialize log
 * @return		#eaf_errno
 */
int eaf_log_init(void);

/**
 * @brief Exit log
 */
void eaf_log_exit(void);

#ifdef __cplusplus
}
#endif
#endif
