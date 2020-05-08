/** @file
 * POSIX regex
 */
#ifndef __EAF_POWERPACK_REGEX_H__
#define __EAF_POWERPACK_REGEX_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "eaf/utils/annotations.h"

/**
 * @brief Regex flags
 */
typedef enum eaf_regex_flag
{
	/**
	 * @brief Use POSIX Extended Regular Expression syntax when interpreting
	 *  regex.
	 *
	 * If not set, POSIX Basic Regular Expression syntax is used.
	 */
	eaf_regex_flag_extened	= 0x01 << 0x00,

	/**
	 * @brief Do not differentiate case.
	 */
	eaf_regex_flag_icase	= 0x01 << 0x01,

	/**
	 * @brief Match-any-character operators don't match a newline.
	 *
	 * A nonmatching list ([^...]) not containing a newline does not match a
	 * newline.
	 *
	 * Match-beginning-of-line operator (^) matches the empty string
	 * immediately after a newline, regardless of whether eflags, the execution
	 * flags of #eaf_regex, contains #eaf_regex_flag_notbol.
	 *
	 * Match-end-of-line operator ($) matches the empty string immediately
	 * before a newline, regardless of whether eflags contains
	 * #eaf_regex_flag_noteol.
	 */
	eaf_regex_flag_newline	= 0x01 << 0x02,

	/**
	 * @brief Do not report position of matches.
	 *
	 * The nmatch and pmatch arguments to #eaf_regex are ignored if the pattern
	 * buffer supplied was compiled with this flag set.
	 */
	eaf_regex_flag_nosub	= 0x01 << 0x03,

	/**
	 * @brief The match-beginning-of-line operator always fails to match (but
	 * see the compilation flag #eaf_regex_flag_newline above)
	 *
	 * This flag may be used when different portions of a string are passed to
	 * #eaf_regex and the beginning of the string should not be interpreted as
	 * the beginning of the line.
	 */
	eaf_regex_flag_notbol	= 0x01 << 0x04,

	/**
	 * @brief The match-end-of-line operator always fails to match (but see the
	 * compilation flag #eaf_regex_flag_newline above)
	 */
	eaf_regex_flag_noteol	= 0x01 << 0x05,
}eaf_regex_flag_t;

/**
 * @brief Match information
 */
typedef struct eaf_regex_match
{
	/**
	 * @brief The start offset of the next largest substring match within the
	 * string
	 */
	long		rm_so;

	/**
	 * @brief The end offset of the match, which is the offset of the first
	 * character after the matching text.
	 */
	long		rm_eo;
}eaf_regex_match_t;

/**
 * @brief Regex
 * @param[in] reg		Regex pattern
 * @param[in] str		The string to be matched
 * @param[in] pmatch	The array to store capture groups
 * @param[in] nmatch	The length of pmatch
 * @param[in] flags		#eaf_regex_flag
 * @return				The number of captured groups
 */
int eaf_regex(_In_ const char* reg, _In_ const char* str,
	_Out_ eaf_regex_match_t pmatch[], _In_ size_t nmatch, _In_ int flags);

#ifdef __cplusplus
}
#endif
#endif
