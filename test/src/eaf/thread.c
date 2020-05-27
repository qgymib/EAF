#include "eaf/eaf.h"
#include "ctest/ctest.h"

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
	ASSERT_NE_PTR(s_eaf_thread_sem, NULL);
}

TEST_CLASS_TEAREDOWN(eaf_thread)
{
	eaf_sem_destroy(s_eaf_thread_sem);
}

TEST_F(eaf_thread, thread)
{
	unsigned long tid = (unsigned long)-1;
	eaf_thread_t* thr = eaf_thread_create(NULL, _test_eaf_thread, &tid);
	ASSERT_NE_PTR(thr, NULL);

	ASSERT_EQ_D32(eaf_sem_pend(s_eaf_thread_sem, 8000), 0);
	ASSERT_NE_U64(tid, eaf_thread_id());

	eaf_thread_destroy(thr);
}
