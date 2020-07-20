/** @file
 * Include this file to use all EAF feature.
 */
#ifndef __EAF_H__
#define __EAF_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup EAF EAF
 * @{
 */
/**
 * @ingroup EAF
 * @defgroup EAF-Core Core
 * @{
 */
#include "eaf/core/message.h"
#include "eaf/core/service.h"
/**
 * @}
 */
/**
 * @ingroup EAF
 * @defgroup EAF-Infra Infra
 * @{
 */
#include "eaf/infra/lock.h"
#include "eaf/infra/semaphore.h"
#include "eaf/infra/thread.h"
/**
 * @}
 */
/**
 * @ingroup EAF
 * @defgroup EAF-Utils Utils
 * @{
 */
#include "eaf/utils/define.h"
#include "eaf/utils/errno.h"
#include "eaf/utils/list.h"
#include "eaf/utils/map.h"
#include "eaf/utils/map_low.h"
/**
 * @}
 */
/**
 * @}
 */

/** @mainpage EAF - Embedded Application Framework
 *
 * # What is EAF
 * EAF is a cross-platform application framework, with focus on resource
 * control.
 *
 * # How to use EAF
 * EAF has a three stage initialize process: `setup`, `publish`, `load`:
 * + In `setup` stage, the organization of thread and service is defined.
 * + In `publish` stage, every service need to register into EAF.
 * + In `load` stage, EAF will try to initialize all service.
 *
 * ## Setup
 * There will be many modules in a program, so how to organize them is a
 * problem. EAF solve this problem by define a thread table. Each module
 * must attach to a pre-defined thread explicitly:
 *
 * ```c
 * // define a thread, with 1 service attached
 * static eaf_service_table_t thread_1[] = {
 *     { TEST_SERVICE_S1, 8 },
 * };
 * // define a thread, with 2 services attached
 * static eaf_service_table_t thread_2[] = {
 *     { TEST_SERVICE_S2, 8 },
 *     { TEST_SERVICE_S3, 8 },
 * };
 * // define thread property
 * static eaf_group_table_t load_table[] = {
 *     { { 0, { 0, 0, 0 } }, { EAF_ARRAY_SIZE(thread_1), thread_1 } },
 *     { { 0, { 0, 0, 0 } }, { EAF_ARRAY_SIZE(thread_2), thread_2 } },
 * };
 * // setup EAF with above thread table
 * eaf_init(load_table, EAF_ARRAY_SIZE(load_table));
 * ```
 *
 *
 * # Publish Service
 *
 * 1. Define your service id
 * ```c
 * #define EXAMPLE_SERVICE_ID	0x00010000
 * ```
 *
 * 2. Define entrypoint
 * ```c
 * static int _example_init(void)
 * {
 *     // initialize code here
 *     return 0;
 * }
 *
 * static void _example_exit(void)
 * {
 *     // cleanup code here
 * }
 *
 * int example_init(void)
 * {
 *     // define entrypoint here.
 *     // note: entrypoint must global accessable
 *     static eaf_entrypoint_t entry = {
 *         0, NULL,
 *         _example_init,
 *         _example_exit,
 *     };
 *     return eaf_register(EXAMPLE_SERVICE_ID, &entry);
 * }
 * ```
 *
 */

#ifdef __cplusplus
}
#endif
#endif  /* __EAF_H__ */
