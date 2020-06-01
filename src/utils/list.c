#include <string.h>
#include "eaf/utils/list.h"

static void _list_lite_set_once(eaf_list_t* handler, eaf_list_node_t* node)
{
	handler->head = node;
	handler->tail = node;
	node->p_after = NULL;
	node->p_before = NULL;
	handler->size = 1;
}

void eaf_list_init(_Out_ eaf_list_t* handler)
{
	memset(handler, 0, sizeof(*handler));
}

void eaf_list_push_back(_Inout_ eaf_list_t* handler, _Inout_ eaf_list_node_t* node)
{
	if (handler->head == NULL)
	{
		_list_lite_set_once(handler, node);
		return;
	}

	node->p_after = NULL;
	node->p_before = handler->tail;
	handler->tail->p_after = node;
	handler->tail = node;
	handler->size++;
}

void eaf_list_insert_before(_Inout_ eaf_list_t* handler, _Inout_ eaf_list_node_t* pos, _Inout_ eaf_list_node_t* node)
{
	if (handler->head == pos)
	{
		eaf_list_push_front(handler, node);
		return;
	}

	node->p_before = pos->p_before;
	node->p_after = pos;
	pos->p_before->p_after = node;
	pos->p_before = node;
	handler->size++;
}

void eaf_list_insert_after(_Inout_ eaf_list_t* handler, _Inout_  eaf_list_node_t* pos, _Inout_ eaf_list_node_t* node)
{
	if (handler->tail == pos)
	{
		eaf_list_push_back(handler, node);
		return;
	}

	node->p_before = pos;
	node->p_after = pos->p_after;
	pos->p_after->p_before = node;
	pos->p_after = node;
	handler->size++;
}

void eaf_list_push_front(_Inout_ eaf_list_t* handler, _Inout_ eaf_list_node_t* node)
{
	if (handler->head == NULL)
	{
		_list_lite_set_once(handler, node);
		return;
	}

	node->p_before = NULL;
	node->p_after = handler->head;
	handler->head->p_before = node;
	handler->head = node;
	handler->size++;
}

eaf_list_node_t* eaf_list_begin(_In_ const eaf_list_t* handler)
{
	return handler->head;
}

eaf_list_node_t* eaf_list_end(_In_ const eaf_list_t* handler)
{
	return handler->tail;
}

eaf_list_node_t* eaf_list_next(_In_ const eaf_list_node_t* node)
{
	return node->p_after;
}

eaf_list_node_t* eaf_list_prev(_In_ const eaf_list_node_t* node)
{
	return node->p_before;
}

void eaf_list_erase(_Inout_ eaf_list_t* handler, _Inout_ eaf_list_node_t* node)
{
	handler->size--;

	/* Î¨Ò»½Úµã */
	if (handler->head == node && handler->tail == node)
	{
		handler->head = NULL;
		handler->tail = NULL;
		return;
	}

	if (handler->head == node)
	{
		node->p_after->p_before = NULL;
		handler->head = node->p_after;
		return;
	}

	if (handler->tail == node)
	{
		node->p_before->p_after = NULL;
		handler->tail = node->p_before;
		return;
	}

	node->p_before->p_after = node->p_after;
	node->p_after->p_before = node->p_before;
	return;
}

eaf_list_node_t* eaf_list_pop_front(_Inout_ eaf_list_t* handler)
{
	eaf_list_node_t* node = handler->head;
	if (node == NULL)
	{
		return NULL;
	}

	eaf_list_erase(handler, node);
	return node;
}

eaf_list_node_t* eaf_list_pop_back(_Inout_ eaf_list_t* handler)
{
	eaf_list_node_t* node = handler->tail;
	if (node == NULL)
	{
		return NULL;
	}

	eaf_list_erase(handler, node);
	return node;
}

size_t eaf_list_size(_In_ const eaf_list_t* handler)
{
	return handler->size;
}
