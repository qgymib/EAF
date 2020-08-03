#include <assert.h>
#include "dial.h"

/*
 * Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
 * https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#	pragma warning(disable : 4127)
#endif

typedef struct dial_record
{
	eaf_list_node_t			node;	/**< List node */

	struct
	{
		volatile int		req;	/**< Request code */
		volatile int		rsp;	/**< Response code */
	}data;

	struct
	{
		volatile unsigned	rsp : 1;
	}sync;
}dial_record_t;

int test_dial_init(test_dial_t* dial)
{
	if ((dial->sync.lock = eaf_lock_create(eaf_lock_attr_normal)) == NULL)
	{
		goto err_create_lock;
	}

	if ((dial->sync.queue = eaf_sem_create(0)) == NULL)
	{
		goto err_create_sync_req;
	}

	dial->code.pend = (eaf_list_t)EAF_LIST_INITIALIZER;
	dial->mask.rsp = 0;

	return 0;

err_create_sync_req:
	eaf_lock_destroy(dial->sync.lock);
err_create_lock:
	return -1;
}

void test_dial_exit(test_dial_t* dial)
{
	assert(eaf_list_size(&dial->code.pend) == 0);

	eaf_sem_destroy(dial->sync.queue);
	eaf_lock_destroy(dial->sync.lock);
}

int test_dial_call(test_dial_t* dial, int req)
{
	dial_record_t record;
	record.data.req = req;
	record.sync.rsp = 0;

	eaf_lock_enter(dial->sync.lock);
	{
		eaf_list_push_back(&dial->code.pend, &record.node);
	}
	eaf_lock_leave(dial->sync.lock);

	eaf_sem_post(dial->sync.queue);

	while (record.sync.rsp == 0)
	{
		eaf_thread_sleep(10);
	}

	return record.data.rsp;
}

int test_dial_wait(test_dial_t* dial, void** token)
{
	eaf_sem_pend(dial->sync.queue, EAF_SEM_INFINITY);

	eaf_list_node_t* it;
	eaf_lock_enter(dial->sync.lock);
	it = eaf_list_pop_front(&dial->code.pend);
	eaf_lock_leave(dial->sync.lock);

	dial_record_t* record = EAF_CONTAINER_OF(it, dial_record_t, node);
	*token = record;

	return record->data.req;
}

void test_dial_answer(test_dial_t* dial, void* token, int rsp)
{
	dial_record_t* record = token;

	eaf_lock_enter(dial->sync.lock);
	dial->code.rsp = rsp;
	dial->mask.rsp = 1;
	eaf_lock_leave(dial->sync.lock);

	record->data.rsp = rsp;
	record->sync.rsp = 1;
}

int test_dial_call_once(test_dial_t* dial, int req)
{
	int rsp = 0;
	int flag_return = 0;
	int flag_wait = 0;

	dial_record_t record;
	record.data.req = req;
	record.data.rsp = 0;
	record.sync.rsp = 0;

	eaf_lock_enter(dial->sync.lock);
	do 
	{
		if (dial->mask.rsp)
		{
			flag_return = 1;
			rsp = dial->code.rsp;
			break;
		}

		if (eaf_list_size(&dial->code.pend) != 0)
		{
			flag_wait = 1;
			break;
		}

		eaf_list_push_back(&dial->code.pend, &record.node);
	} while (0);
	eaf_lock_leave(dial->sync.lock);

	if (flag_return)
	{
		return rsp;
	}

	if (flag_wait)
	{
		eaf_lock_enter(dial->sync.lock);
		while (dial->mask.rsp == 0)
		{
			eaf_lock_leave(dial->sync.lock);
			eaf_thread_sleep(10);
			eaf_lock_enter(dial->sync.lock);
		}
		rsp = dial->code.rsp;
		eaf_lock_leave(dial->sync.lock);

		return rsp;
	}

	while (record.sync.rsp == 0)
	{
		eaf_thread_sleep(10);
	}
	return record.data.rsp;
}
