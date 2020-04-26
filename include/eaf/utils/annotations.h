/** @file
 * annotations.h provides a set of annotations to describe how a function uses
 * its parameters - the assumptions it makes about them, and the guarantees it
 * makes upon finishing.
 */
#ifndef __EAF_UTILS_ANNOTATIONS_H__
#define __EAF_UTILS_ANNOTATIONS_H__
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#include <sal.h>
#else

/**
 * Annotates input parameters that are scalars, structures, pointers to
 * structures and the like. Explicitly may be used on simple scalars. The
 * parameter must be valid in pre-state and will not be modified.
 */
#define _In_

/**
 * Annotates output parameters that are scalars, structures, pointers to
 * structures and the like. Don't apply this annotation to an object that
 * can't return a value—for example, a scalar that's passed by value. The
 * parameter doesn't have to be valid in pre-state but must be valid in
 * post-state.
 */
#define _Out_

/**
 * Like #_Out_, but parameters are optional.
 * @see \_Out\_
 */
#define _Out_opt_

/**
 * Parameter can't be null, and in the post-state the pointed-to location can't
 * be null and must be valid.
 */
#define _Outptr_

/**
 * Parameter may be null, but in the post-state the pointed-to location can't
 * be null and must be valid.
 */
#define _Outptr_opt_

/**
 * Parameter may be null, and in the post-state the pointed-to location can be
 * null.
 */
#define _Outptr_opt_result_maybenull_

/**
 * Annotates a parameter that will be changed by the function. It must be valid
 * in both pre-state and post-state, but is assumed to have different values
 * before and after the call. Must apply to a modifiable value.
 */
#define _Inout_

/**
 * Like #_Inout_, but parameters are optional.
 * @see \_Inout\_
 */
#define _Inout_opt_

/**
 * Annotates a parameter that will be invalid when the function returns.
 */
#define _Post_invalid_

#endif

#ifdef __cplusplus
}
#endif
#endif
