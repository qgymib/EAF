#include "errno.h"

typedef struct eaf_errno_table
{
	eaf_errno_t		err;	/** ������ */
	const char*		str;	/** ���� */
}eaf_errno_table_t;

static eaf_errno_table_t err_table[] = {
#define __EAF_HEADER_MULTI_READ__
#undef EAF_DEFINE_ERRNO
#define EAF_DEFINE_ERRNO(err, describe) { err, describe },
#include "errno.h"
#undef __EAF_HEADER_MULTI_READ__
};

const char* eaf_strerror(_In_ int err)
{
	size_t i;
	for (i = 0; i < (sizeof(err_table) / sizeof(err_table[0])); i++)
	{
		if (err == (int)err_table[i].err)
		{
			return err_table[i].str;
		}
	}

	return NULL;
}
