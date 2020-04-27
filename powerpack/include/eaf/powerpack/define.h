/** @file
 * Powerpack Macros.
 */
#ifndef __EAF_POWERPACK_DEFINE_H__
#define __EAF_POWERPACK_DEFINE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def MSVC_PUSH_WARNNING(x)
 * @brief Disable specific warning for msvc
 * @param[in] x The warning need to disable
 */

/**
 * @def MSVC_POP_WARNNING()
 * @brief Restore previous warning state
 */

#if defined(_MSC_VER)
#	define MSVC_PUSH_WARNNING(x)	\
		__pragma(warning(push))\
		__pragma(warning(disable : x))
#	define MSVC_POP_WARNNING()	\
		__pragma(warning(pop))
#else
#	define MSVC_PUSH_WARNNING(x)
#	define MSVC_POP_WARNNING()
#endif

#ifdef __cplusplus
}
#endif
#endif
