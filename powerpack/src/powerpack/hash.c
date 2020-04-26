#include "hash.h"

uint32_t eaf_hash32_bkdr(_In_ const void* data, _In_ size_t size,
	_In_ uint32_t seed)
{
	const char* p_dat = (const char*)data;

	size_t i;
	for (i = 0; i < size; i++)
	{
		seed = seed * 131313 + p_dat[i];
	}

	return seed;
}
