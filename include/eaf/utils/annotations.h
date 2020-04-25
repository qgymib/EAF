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
 * Data is passed to the called function, and is treated as read-only.
 */
#define _In_

/**
 * The caller only provides space for the called function to write to. The
 * called function writes data into that space.
 */
#define _Out_

/**
 * Like #_Out_, but parameters are optional.
 */
#define _Out_opt_

/**
 * Like #_Out_. The value that's returned by the called function is a pointer.
 */
#define _Outptr_

/**
 * like #_Outptr_, except param is reference-to-pointer type.
 * @see \_Outptr\_
 */
#define _Outref_

/**
 * Usable data is passed into the function and potentially is modified.
 */
#define _Inout_

/**
 * Like #_Inout_, but parameters are optional.
 * @see \_Inout\_
 */
#define _Inout_opt_

/**
 * for return values
 */
#define _Ret_

/**
 * class/struct field invariants
 */
#define _Field_

#endif

#ifdef __cplusplus
}
#endif
#endif
