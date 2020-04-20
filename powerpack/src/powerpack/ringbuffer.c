#include "EAF/utils/define.h"
#include "EAF/powerpack/ringbuffer.h"

typedef enum ring_buffer_node_state
{
	stat_writing,										/** ����writing״̬ */
	stat_committed,										/** ����committed״̬ */
	stat_reading,										/** ����reading״̬ */
}ring_buffer_node_state_t;

typedef struct ring_buffer_node
{
	struct
	{
		struct ring_buffer_node*	p_forward;			/** ������·�е���һλ�á��κ�����¶���ΪNULL */
		struct ring_buffer_node*	p_backward;			/** ������·�е���һλ�á��κ�����¶���ΪNULL */
	}chain_pos;

	struct
	{
		struct ring_buffer_node*	p_newer;			/** ���½ڵ㡣������ʱΪNULL */
		struct ring_buffer_node*	p_older;			/** ���Ͻڵ㡣������ʱΪNULL */
	}chain_time;

	ring_buffer_node_state_t		state;				/** �ڵ�״̬ */
	eaf_ringbuffer_token_t			token;				/** �û����� */
}ring_buffer_node_t;

struct eaf_ringbuffer
{
	struct
	{
		uint8_t*					cache;				/** ��ʼ���õ�ַ����һ�����ڸ�������ʼ��ַ */
		size_t						capacity;			/** �������ݳ��� */
	}config;

	struct
	{
		ring_buffer_node_t*			HEAD;				/** ָ�����µĴ��� reading/writing/committed ״̬�Ľڵ� */
		ring_buffer_node_t*			TAIL;				/** ָ�����ϵĴ��� reading/writing/committed ״̬�Ľڵ� */
		ring_buffer_node_t*			oldest_reserve;		/** ָ�������ϵ� writing/committed ״̬�Ľڵ� */
	}node;

	eaf_ringbuffer_counter_t			counter;			/** ������ */
};

static void _ring_buffer_reinit(eaf_ringbuffer_t* rb)
{
	rb->counter.writing = 0;
	rb->counter.committed = 0;
	rb->counter.reading = 0;

	rb->node.oldest_reserve = NULL;
	rb->node.HEAD = NULL;
	rb->node.TAIL = NULL;
}

/**
* �ڿյ�RingBuffer�д�����һ���ڵ�
* @param rb			ring buffer
* @param data_len	length of user data
* @param node_size	size of node
* @return			token
*/
static eaf_ringbuffer_token_t* _ring_buffer_reserve_empty(eaf_ringbuffer_t* rb,
	size_t data_len, size_t node_size)
{
	/* check capacity */
	if (node_size > rb->config.capacity)
	{
		return NULL;
	}

	/* ��ʼ���ڵ� */
	rb->node.HEAD = (ring_buffer_node_t*)rb->config.cache;
	rb->node.HEAD->state = stat_writing;
	rb->node.HEAD->token.size.size = data_len;

	/* ��ʼ��λ���� */
	rb->node.HEAD->chain_pos.p_forward = rb->node.HEAD;
	rb->node.HEAD->chain_pos.p_backward = rb->node.HEAD;

	/* ��ʼ��ʱ���� */
	rb->node.HEAD->chain_time.p_newer = NULL;
	rb->node.HEAD->chain_time.p_older = NULL;

	/* ��ʼ��������Դ */
	rb->node.TAIL = rb->node.HEAD;
	rb->node.oldest_reserve = rb->node.HEAD;
	rb->counter.writing++;

	return &rb->node.oldest_reserve->token;
}

/**
* Ϊnew_node����chain_time
* @param rb	RingBuffer
* @param new_node	�½ڵ�
*/
static void _ring_buffer_update_time_for_new_node(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* new_node)
{
	/* update chain_time */
	new_node->chain_time.p_newer = NULL;
	new_node->chain_time.p_older = rb->node.HEAD;
	new_node->chain_time.p_older->chain_time.p_newer = new_node;

	/* update HEAD */
	rb->node.HEAD = new_node;
}

/**
* ִ��overwrite����
*/
static eaf_ringbuffer_token_t* _ring_buffer_perform_overwrite(
	eaf_ringbuffer_t* rb, uint8_t* start_point, ring_buffer_node_t* node_start,
	ring_buffer_node_t* node_end, size_t counter_lost_nodes,
	size_t data_len)
{
	ring_buffer_node_t* newer_node = node_end->chain_time.p_newer;

	/*
	 * here [node_start, node_end] will be overwrite,
	 * so oldest_reserve need to move forward.
	 * if TAIL was overwrite, then move TAIL too.
	 */
	if (rb->node.TAIL == rb->node.oldest_reserve)
	{
		rb->node.TAIL = newer_node;
	}
	rb->node.oldest_reserve = newer_node;

	/* generate new node */
	ring_buffer_node_t* new_node = (ring_buffer_node_t*)start_point;

	/* ����λ���� */
	new_node->chain_pos.p_forward = node_end->chain_pos.p_forward;
	new_node->chain_pos.p_forward->chain_pos.p_backward = new_node;
	new_node->chain_pos.p_backward = node_start->chain_pos.p_backward;
	new_node->chain_pos.p_backward->chain_pos.p_forward = new_node;

	/* ����ʱ���� */
	if (node_start->chain_time.p_older != NULL)
	{
		node_start->chain_time.p_older->chain_time.p_newer =
			node_end->chain_time.p_newer;
	}
	if (node_end->chain_time.p_newer != NULL)
	{
		node_end->chain_time.p_newer->chain_time.p_older =
			node_start->chain_time.p_older;
	}
	_ring_buffer_update_time_for_new_node(rb, new_node);

	/* ���¼����� */
	rb->counter.committed -= counter_lost_nodes;
	rb->counter.writing += 1;

	/* ���³��� */
	new_node->token.size.size = data_len;

	/* ���Ľڵ�״̬ */
	new_node->state = stat_writing;

	return &new_node->token;
}

/**
* try to overwrite
*/
static eaf_ringbuffer_token_t* _ring_buffer_reserve_try_overwrite(
	eaf_ringbuffer_t* rb, size_t data_len, size_t node_size)
{
	/* overwrite���Դ���committed״̬�Ľڵ���Ч */
	if (rb->node.oldest_reserve == NULL
		|| rb->node.oldest_reserve->state != stat_committed)
	{
		return NULL;
	}

	/* ����ǰ������һ��node����������ringbuffer�Ƿ���������������� */
	if (rb->node.oldest_reserve->chain_pos.p_forward == rb->node.oldest_reserve)
	{
		/* ringbuffer������������������ */
		if (rb->config.capacity < node_size)
		{
			return NULL;
		}

		/* ������������ʱ�����³�ʼ������ӽڵ� */
		_ring_buffer_reinit(rb);
		return _ring_buffer_reserve_empty(rb, data_len, node_size);
	}

	/* ��1��������overwrite��������ʼλ�� */
	const ring_buffer_node_t* backward_node =
		rb->node.oldest_reserve->chain_pos.p_backward;
	uint8_t* start_point = (backward_node < rb->node.oldest_reserve) ?
		((uint8_t*)backward_node + eaf_ringbuffer_node_cost(backward_node->token.size.size)) :
		(rb->config.cache);

	/* ��2�������������Ĵ���committed״̬�Ľڵ��ܷ������������� */
	size_t sum_size = 0;
	size_t counter_lost_nodes = 1;
	ring_buffer_node_t* node_end = rb->node.oldest_reserve;

	for (;;)
	{
		sum_size = (uint8_t*)node_end
			+ eaf_ringbuffer_node_cost(node_end->token.size.size)
			- (uint8_t*)start_point;
		if (!(sum_size < node_size /* overwrite minimum nodes */
			&& node_end->chain_pos.p_forward->state == stat_committed /* only overwrite committed node */
			&& node_end->chain_pos.p_forward == node_end->chain_time.p_newer /* node must both physical and time continuous */
			&& node_end->chain_pos.p_forward > node_end /* cannot interrupt by array boundary */
			))
		{
			break;
		}
		node_end = node_end->chain_pos.p_forward;
		counter_lost_nodes++;
	}

	/* ��3��������Ƿ�߱�overwrite������ */
	if (sum_size < node_size)
	{
		return NULL;
	}

	/* ��4����ִ��overwrite */
	return _ring_buffer_perform_overwrite(rb, start_point, rb->node.oldest_reserve,
		node_end, counter_lost_nodes, data_len);
}

static void _ring_buffer_insert_new_node(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* new_node, size_t data_len)
{
	/* initialize token */
	new_node->state = stat_writing;
	new_node->token.size.size = data_len;

	/* update chain_pos */
	new_node->chain_pos.p_forward = rb->node.HEAD->chain_pos.p_forward;
	new_node->chain_pos.p_backward = rb->node.HEAD;
	new_node->chain_pos.p_forward->chain_pos.p_backward = new_node;
	new_node->chain_pos.p_backward->chain_pos.p_forward = new_node;

	_ring_buffer_update_time_for_new_node(rb, new_node);
}

/**
* ����oldest_reserve
*/
static void _ring_buffer_reserve_update_oldest_reserve(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* node)
{
	if (rb->node.oldest_reserve == NULL)
	{
		rb->node.oldest_reserve = node;
	}
}

static eaf_ringbuffer_token_t* _ring_buffer_reserve_none_empty(
	eaf_ringbuffer_t* rb, size_t data_len, size_t node_size, int flags)
{
	/*
	 * HEAD�Ҳ����һ�����ܵĽڵ��ַ
	 * ��HEAD�Ҳ���ڽڵ㣬����ڵ��ַһ�����ڵ��ڼ���ó��Ľڵ��ַ
	 */
	ring_buffer_node_t* next_possible_node =
		(ring_buffer_node_t*)((uint8_t*)rb->node.HEAD
		+ eaf_ringbuffer_node_cost(rb->node.HEAD->token.size.size));

	/* ���HEAD�Ҳ���������ڵ㣬�����������ڵ�֮ǰ�γ�token */
	if (rb->node.HEAD->chain_pos.p_forward > rb->node.HEAD)
	{
		/* ��HEAD�Ҳ�ڵ���HEAD֮��Ŀ�϶�㹻ʱ�����γ�token */
		if ((size_t)((uint8_t*)rb->node.HEAD->chain_pos.p_forward
			- (uint8_t*)next_possible_node) >= node_size)
		{
			rb->counter.writing++;
			_ring_buffer_insert_new_node(rb, next_possible_node, data_len);
			_ring_buffer_reserve_update_oldest_reserve(rb, next_possible_node);
			return &next_possible_node->token;
		}

		/* �����������½���overwrite */
		return (flags & eaf_ringbuffer_flag_overwrite) ?
			_ring_buffer_reserve_try_overwrite(rb, data_len, node_size) :
			NULL;
	}

	/* HEAD�Ҳ಻���ڽڵ�ʱ����HEAD�Ҳ�ռ��㹻�����γ�token */
	if ((rb->config.capacity
		- ((uint8_t*)next_possible_node - rb->config.cache)) >= node_size)
	{
		rb->counter.writing++;
		_ring_buffer_insert_new_node(rb, next_possible_node, data_len);
		_ring_buffer_reserve_update_oldest_reserve(rb, next_possible_node);
		return &next_possible_node->token;
	}

	/* if area on the most left cache is enough, make token */
	if ((size_t)((uint8_t*)rb->node.HEAD->chain_pos.p_forward - rb->config.cache)
		>= node_size)
	{
		next_possible_node = (ring_buffer_node_t*)rb->config.cache;
		rb->counter.writing++;
		_ring_buffer_insert_new_node(rb, next_possible_node, data_len);
		_ring_buffer_reserve_update_oldest_reserve(rb, next_possible_node);
		return &next_possible_node->token;
	}

	/* in other condition, overwrite if needed */
	return (flags & eaf_ringbuffer_flag_overwrite) ?
		_ring_buffer_reserve_try_overwrite(rb, data_len, node_size) : NULL;
}

static int _ring_buffer_commit_for_write_confirm(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* node)
{
	(void)rb;

	/* ���¼����� */
	rb->counter.writing--;
	rb->counter.committed++;

	/* �޸Ľڵ�״̬ */
	node->state = stat_committed;

	return 0;
}

/**
* ɾ��nodeʱ������ chain_pos �� chain_time
* @param node	��ɾ���ڵ�
*/
static void _ring_buffer_delete_node_update_chain(
	ring_buffer_node_t* node)
{
	/* ���� chain_pos */
	node->chain_pos.p_backward->chain_pos.p_forward = node->chain_pos.p_forward;
	node->chain_pos.p_forward->chain_pos.p_backward =
		node->chain_pos.p_backward;

	/* ���� chain_time */
	if (node->chain_time.p_older != NULL)
	{
		node->chain_time.p_older->chain_time.p_newer = node->chain_time.p_newer;
	}
	if (node->chain_time.p_newer != NULL)
	{
		node->chain_time.p_newer->chain_time.p_older = node->chain_time.p_older;
	}
}

/**
* completely remove a node from ring buffer
* @param rb	ring buffer
* @param node	node to be delete
*/
static void _ring_buffer_delete_node(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* node)
{
	/**
	 * ��RingBuffer�н�����һ���ڵ�ʱ������RingBuffer
	 * �˴�ʹ�� p_forward ���ж��Ƿ������һ���ڵ㣬ԭ�����£�
	 * 1. chain_pos.p_forward ����ָ������·�е���һ�ڵ㣬����������ʱ��������·��һ��ֻ��һ���ڵ�
	 * 2. �� node ���� cacheline ��ʱ��p_forward ��Ϊ node �ĵ�һ��Ԫ�أ�һ��Ҳ���� cacheline �У����Ա�������ڴ�
	 */
	if (node->chain_pos.p_forward == node)
	{
		_ring_buffer_reinit(rb);
		return;
	}

	/* ���� chain_pos �� chain_time */
	_ring_buffer_delete_node_update_chain(node);

	/* ���� oldest_reserve */
	if (rb->node.oldest_reserve == node)
	{
		rb->node.oldest_reserve = node->chain_time.p_newer;
	}

	/*
	 * node Ϊ TAIL �ڵ�ʱ������ TAIL
	 */
	if (node->chain_time.p_older == NULL)
	{
		rb->node.TAIL = node->chain_time.p_newer;
		return;
	}

	/*
	 * node Ϊ HEAD �ڵ�ʱ������ HEAD
	 */
	if (node->chain_time.p_newer == NULL)
	{
		rb->node.HEAD = node->chain_time.p_older;
		return;
	}

	return;
}

static int _ring_buffer_commit_for_write_discard(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* node)
{
	rb->counter.writing--;
	_ring_buffer_delete_node(rb, node);
	return 0;
}

static int _ring_buffer_commit_for_write(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* node, int flags)
{
	return (flags & eaf_ringbuffer_flag_discard) ?
		_ring_buffer_commit_for_write_discard(rb, node) :
		_ring_buffer_commit_for_write_confirm(rb, node);
}

static int _ring_buffer_commit_for_consume_confirm(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* node)
{
	rb->counter.reading--;
	_ring_buffer_delete_node(rb, node);
	return 0;
}

/**
* discard a consumed token.
* the only condition a consumed token can be discard is no one consume newer token
*/
static int _ring_buffer_commit_for_consume_discard(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* node, int flags)
{
	/* ��������µ������ߣ���Ӧ��ʧ�� */
	if (node->chain_time.p_newer != NULL
		&& node->chain_time.p_newer->state == stat_reading)
	{
		return (flags & eaf_ringbuffer_flag_abandon) ?
			_ring_buffer_commit_for_consume_confirm(rb, node) : -1;
	}

	/* �޸ļ����� */
	rb->counter.reading--;
	rb->counter.committed++;

	/* �޸�״̬ */
	node->state = stat_committed;

	/* if no newer node, then oldest_reserve should point to this node */
	if (node->chain_time.p_newer == NULL)
	{
		rb->node.oldest_reserve = node;
		return 0;
	}

	/* if node is just older than oldest_reserve, then oldest_reserve should move back */
	if (rb->node.oldest_reserve != NULL
		&& rb->node.oldest_reserve->chain_time.p_older == node)
	{
		rb->node.oldest_reserve = node;
		return 0;
	}

	return 0;
}

static int _ring_buffer_commit_for_consume(eaf_ringbuffer_t* rb,
	ring_buffer_node_t* node, int flags)
{
	return (flags & eaf_ringbuffer_flag_discard) ?
		_ring_buffer_commit_for_consume_discard(rb, node, flags) :
		_ring_buffer_commit_for_consume_confirm(rb, node);
}

size_t eaf_ringbuffer_heap_cost(void)
{
	/* need to align with machine size */
	return EAF_ALIGN(sizeof(struct eaf_ringbuffer), sizeof(void*));
}

size_t eaf_ringbuffer_node_cost(size_t size)
{
	return EAF_ALIGN(sizeof(ring_buffer_node_t) + size, sizeof(void*));
}

eaf_ringbuffer_t* eaf_ringbuffer_init(void* buffer, size_t size)
{
	/* ����ͷ����ʼ�ص� */
	eaf_ringbuffer_t* handler = (void*)EAF_ALIGN(buffer, sizeof(void*));

	/* ����Ϊ�˶��뵼�µĿ�϶��С */
	const size_t leading_align_size = (uint8_t*)handler - (uint8_t*)buffer;

	/** ���ʣ�������Ƿ���� */
	if (leading_align_size + eaf_ringbuffer_heap_cost() >= size)
	{
		return NULL;
	}

	/* setup necessary field */
	handler->config.cache = (uint8_t*)handler + eaf_ringbuffer_heap_cost();
	handler->config.capacity = size - leading_align_size - eaf_ringbuffer_heap_cost();

	/* initialize */
	_ring_buffer_reinit(handler);

	return handler;
}

eaf_ringbuffer_token_t* eaf_ringbuffer_reserve(eaf_ringbuffer_t* handler, size_t len,
	int flags)
{
	/* node must aligned */
	const size_t node_size = eaf_ringbuffer_node_cost(len);

	/* empty ring buffer */
	if (handler->node.TAIL == NULL)
	{
		return _ring_buffer_reserve_empty(handler, len, node_size);
	}

	/* non empty ring buffer */
	return _ring_buffer_reserve_none_empty(handler, len, node_size, flags);
}

eaf_ringbuffer_token_t* eaf_ringbuffer_consume(eaf_ringbuffer_t* handler)
{
	if (handler->node.oldest_reserve == NULL
		|| handler->node.oldest_reserve->state != stat_committed)
	{
		return NULL;
	}

	handler->counter.committed--;
	handler->counter.reading++;

	ring_buffer_node_t* token_node = handler->node.oldest_reserve;
	handler->node.oldest_reserve = handler->node.oldest_reserve->chain_time.p_newer;
	token_node->state = stat_reading;

	return &token_node->token;
}

int eaf_ringbuffer_commit(eaf_ringbuffer_t* handler, eaf_ringbuffer_token_t* token, int flags)
{
	ring_buffer_node_t* node = EAF_CONTAINER_OF(token, ring_buffer_node_t, token);

	return node->state == stat_writing ?
		_ring_buffer_commit_for_write(handler, node, flags) :
		_ring_buffer_commit_for_consume(handler, node, flags);
}

size_t eaf_ringbuffer_count(eaf_ringbuffer_t* handler, eaf_ringbuffer_counter_t* counter)
{
	if (counter != NULL)
	{
		*counter = handler->counter;
	}

	return handler->counter.committed + handler->counter.reading + handler->counter.writing;
}

size_t eaf_ringbuffer_foreach(eaf_ringbuffer_t* handler,
	int(*cb)(const eaf_ringbuffer_token_t* token, void* arg), void* arg)
{
	size_t counter = 0;
	ring_buffer_node_t* it = handler->node.TAIL;
	for (; it != NULL; it = it->chain_time.p_newer)
	{
		if (cb(&it->token, arg) < 0)
		{
			break;
		}
		counter++;
	}

	return counter;
}
