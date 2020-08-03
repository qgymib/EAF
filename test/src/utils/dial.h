#ifndef __TEST_UTILS_DIAL_H__
#define __TEST_UTILS_DIAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "eaf/eaf.h"

typedef struct test_dial
{
	struct
	{
		unsigned		rsp : 1;
	}mask;

	struct
	{
		int				rsp;
		eaf_list_t		pend;
	}code;

	struct
	{
		eaf_sem_t*		queue;
		eaf_lock_t*		lock;
	}sync;
}test_dial_t;

/**
 * @brief Initialize dial
 * @param[in] dial	Handler
 * @return			#eaf_errno
 */
int test_dial_init(test_dial_t* dial);

/**
 * @brief Destroy dial
 * @param[in] dial	A pointer to the dial that already initialized
 */
void test_dial_exit(test_dial_t* dial);

/**
 * @brief Dial out and wait for response
 * @param[in] dial	Dial handler
 * @param[in] req	Request code
 * @return			Response code
 */
int test_dial_call(test_dial_t* dial, int req);

/**
 * @brief Dial out and wait for response.
 *
 * If already dialed, the previous response is returned.
 *
 * @param[in] dial	Dial handler
 * @param[in] req	Request code
 * @return			Response code
 */
int test_dial_call_once(test_dial_t* dial, int req);

/**
 * @brief Wait for dial
 * @param[in] dial		Dial handler
 * @param[out] token	Dial token
 * @return				Request code
 */
int test_dial_wait(test_dial_t* dial, void** token);

/**
 * @brief Answer a dial
 * @param[in] dial		Dial handler
 * @param[in] token		Dial token
 * @param[in] rsp		Response code
 */
void test_dial_answer(test_dial_t* dial, void* token, int rsp);

#ifdef __cplusplus
}
#endif
#endif
