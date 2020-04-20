#include "EAF/eaf.h"
#include "etest/etest.h"

static eaf_sem_t* s_eaf_thread_sem;

static void _test_eaf_thread(void* arg)
{
	unsigned long* tid = arg;
	*tid = eaf_thread_id();

	eaf_sem_post(s_eaf_thread_sem);
}

TEST_CLASS_SETUP(eaf_thread)
{
	s_eaf_thread_sem = eaf_sem_create(0);
	ASSERT_PTR_NE(s_eaf_thread_sem, NULL);
}

TEST_CLASS_TEAREDOWN(eaf_thread)
{
	eaf_sem_destroy(s_eaf_thread_sem);
}

TEST_F(eaf_thread, thread)
{
	unsigned long tid = (unsigned long)-1;
	eaf_thread_t* thr = eaf_thread_create(NULL, _test_eaf_thread, &tid);
	ASSERT_PTR_NE(thr, NULL);

	ASSERT_NUM_EQ(eaf_sem_pend(s_eaf_thread_sem, 8000), 0);
	ASSERT_NUM_NE(tid, eaf_thread_id());

	eaf_thread_destroy(thr);
}
