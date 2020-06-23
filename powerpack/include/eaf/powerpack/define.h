/** @file
 * PowerPack Macros.
 */
#ifndef __EAF_POWERPACK_DEFINE_H__
#define __EAF_POWERPACK_DEFINE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def EAF_MSVC_PUSH_WARNNING(x)
 * @brief Disable specific warning for msvc
 * @param[in] x The warning need to disable
 */

/**
 * @def EAF_MSVC_POP_WARNNING()
 * @brief Restore previous warning state
 */

#if defined(_MSC_VER)
#	define EAF_MSVC_PUSH_WARNNING(x)	\
		__pragma(warning(push))\
		__pragma(warning(disable : x))
#	define EAF_MSVC_POP_WARNNING()	\
		__pragma(warning(pop))
#else
#	define EAF_MSVC_PUSH_WARNNING(x)
#	define EAF_MSVC_POP_WARNNING()
#endif

#ifdef __cplusplus
}
#endif
#endif
