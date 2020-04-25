#include "eaf/utils/errno.h"
#include "eaf/utils/map.h"

void eaf_map_init(_Out_ eaf_map_t* handler, _In_ eaf_map_cmp_fn cmp, _Inout_opt_ void* arg)
{
	handler->map_low = EAF_MAP_LOW_INIT;
	handler->cmp.cmp = cmp;
	handler->cmp.arg = arg;
	handler->size = 0;
}

int eaf_map_insert(_Inout_ eaf_map_t* handler, _Inout_ eaf_map_node_t* node)
{
	eaf_map_low_node_t **new_node = &(handler->map_low.rb_root), *parent = NULL;

	/* Figure out where to put new node */
	while (*new_node)
	{
		int result = handler->cmp.cmp(node, *new_node, handler->cmp.arg);

		parent = *new_node;
		if (result < 0)
		{
			new_node = &((*new_node)->rb_left);
		}
		else if (result > 0)
		{
			new_node = &((*new_node)->rb_right);
		}
		else
		{
			return eaf_errno_duplicate;
		}
	}

	handler->size++;
	eaf_map_low_link_node(node, parent, new_node);
	eaf_map_low_insert_color(node, &handler->map_low);

	return eaf_errno_success;
}

void eaf_map_erase(_Inout_ eaf_map_t* handler, _Inout_  eaf_map_node_t* node)
{
	handler->size--;
	eaf_map_low_erase(&handler->map_low, node);
}

size_t eaf_map_size(_In_ const eaf_map_t* handler)
{
	return handler->size;
}

eaf_map_node_t* eaf_map_find(_In_ const eaf_map_t* handler, _In_ const eaf_map_node_t* key)
{
	eaf_map_low_node_t* node = handler->map_low.rb_root;

	while (node)
	{
		int result = handler->cmp.cmp(key, node, handler->cmp.arg);

		if (result < 0)
		{
			node = node->rb_left;
		}
		else if (result > 0)
		{
			node = node->rb_right;
		}
		else
		{
			return node;
		}
	}

	return NULL;
}

eaf_map_node_t* eaf_map_find_lower(_In_ const eaf_map_t* handler, _In_  const eaf_map_node_t* key)
{
	eaf_map_node_t* lower_node = NULL;
	eaf_map_node_t* node = handler->map_low.rb_root;
	while (node)
	{
		int result = handler->cmp.cmp(key, node, handler->cmp.arg);
		if (result < 0)
		{
			node = node->rb_left;
		}
		else if (result > 0)
		{
			lower_node = node;
			node = node->rb_right;
		}
		else
		{
			return node;
		}
	}

	return lower_node;
}

eaf_map_node_t* eaf_map_find_upper(_In_ const eaf_map_t* handler, _In_  const eaf_map_node_t* key)
{
	eaf_map_node_t* upper_node = NULL;
	eaf_map_node_t* node = handler->map_low.rb_root;

	while (node)
	{
		int result = handler->cmp.cmp(key, node, handler->cmp.arg);

		if (result < 0)
		{
			upper_node = node;
			node = node->rb_left;
		}
		else if (result > 0)
		{
			node = node->rb_right;
		}
		else
		{
			if (upper_node == NULL)
			{
				upper_node = node->rb_right;
			}
			break;
		}
	}

	return upper_node;
}

eaf_map_node_t* eaf_map_begin(_In_ const eaf_map_t* handler)
{
	return eaf_map_low_first(&handler->map_low);
}

eaf_map_node_t* eaf_map_end(_In_ const eaf_map_t* handler)
{
	return eaf_map_low_last(&handler->map_low);
}

eaf_map_node_t* eaf_map_next(_In_ const eaf_map_t* handler, _In_  const eaf_map_node_t* node)
{
	(void)handler;
	return eaf_map_low_next(node);
}

eaf_map_node_t* eaf_map_prev(_In_ const eaf_map_t* handler, _In_ const eaf_map_node_t* node)
{
	(void)handler;
	return eaf_map_low_prev(node);
}
