#include "EAF/utils/define.h"
#include "EAF/powerpack/ringbuffer.h"

typedef enum ring_buffer_node_state
{
	stat_writing,										/** 处于writing状态 */
	stat_committed,										/** 处于committed状态 */
	stat_reading,										/** 处于reading状态 */
}ring_buffer_node_state_t;

typedef struct ring_buffer_node
{
	struct
	{
		struct ring_buffer_node*	p_forward;			/** 环形链路中的下一位置。任何情况下都不为NULL */
		struct ring_buffer_node*	p_backward;			/** 环形链路中的上一位置。任何情况下都不为NULL */
	}chain_pos;

	struct
	{
		struct ring_buffer_node*	p_newer;			/** 较新节点。不存在时为NULL */
		struct ring_buffer_node*	p_older;			/** 较老节点。不存在时为NULL */
	}chain_time;

	ring_buffer_node_state_t		state;				/** 节点状态 */
	eaf_ringbuffer_token_t			token;				/** 用户数据 */
}ring_buffer_node_t;

struct eaf_ringbuffer
{
	struct
	{
		uint8_t*					cache;				/** 起始可用地址。不一定等于给定的起始地址 */
		size_t						capacity;			/** 可用数据长度 */
	}config;

	struct
	{
		ring_buffer_node_t*			HEAD;				/** 指向最新的处于 reading/writing/committed 状态的节点 */
		ring_buffer_node_t*			TAIL;				/** 指向最老的处于 reading/writing/committed 状态的节点 */
		ring_buffer_node_t*			oldest_reserve;		/** 指向处于最老的 writing/committed 状态的节点 */
	}node;

	eaf_ringbuffer_counter_t			counter;			/** 计数器 */
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
* 在空的RingBuffer中创建第一个节点
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

	/* 初始化节点 */
	rb->node.HEAD = (ring_buffer_node_t*)rb->config.cache;
	rb->node.HEAD->state = stat_writing;
	rb->node.HEAD->token.size.size = data_len;

	/* 初始化位置链 */
	rb->node.HEAD->chain_pos.p_forward = rb->node.HEAD;
	rb->node.HEAD->chain_pos.p_backward = rb->node.HEAD;

	/* 初始化时间链 */
	rb->node.HEAD->chain_time.p_newer = NULL;
	rb->node.HEAD->chain_time.p_older = NULL;

	/* 初始化其他资源 */
	rb->node.TAIL = rb->node.HEAD;
	rb->node.oldest_reserve = rb->node.HEAD;
	rb->counter.writing++;

	return &rb->node.oldest_reserve->token;
}

/**
* 为new_node更新chain_time
* @param rb	RingBuffer
* @param new_node	新节点
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
* 执行overwrite操作
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

	/* 更新位置链 */
	new_node->chain_pos.p_forward = node_end->chain_pos.p_forward;
	new_node->chain_pos.p_forward->chain_pos.p_backward = new_node;
	new_node->chain_pos.p_backward = node_start->chain_pos.p_backward;
	new_node->chain_pos.p_backward->chain_pos.p_forward = new_node;

	/* 更新时间链 */
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

	/* 更新计数器 */
	rb->counter.committed -= counter_lost_nodes;
	rb->counter.writing += 1;

	/* 更新长度 */
	new_node->token.size.size = data_len;

	/* 更改节点状态 */
	new_node->state = stat_writing;

	return &new_node->token;
}

/**
* try to overwrite
*/
static eaf_ringbuffer_token_t* _ring_buffer_reserve_try_overwrite(
	eaf_ringbuffer_t* rb, size_t data_len, size_t node_size)
{
	/* overwrite仅对处于committed状态的节点有效 */
	if (rb->node.oldest_reserve == NULL
		|| rb->node.oldest_reserve->state != stat_committed)
	{
		return NULL;
	}

	/* 若当前仅存在一个node，则检查整个ringbuffer是否可以容纳所需数据 */
	if (rb->node.oldest_reserve->chain_pos.p_forward == rb->node.oldest_reserve)
	{
		/* ringbuffer不足以容纳所需数据 */
		if (rb->config.capacity < node_size)
		{
			return NULL;
		}

		/* 可以容纳数据时，重新初始化并添加节点 */
		_ring_buffer_reinit(rb);
		return _ring_buffer_reserve_empty(rb, data_len, node_size);
	}

	/* 第1步：计算overwrite产生的起始位置 */
	const ring_buffer_node_t* backward_node =
		rb->node.oldest_reserve->chain_pos.p_backward;
	uint8_t* start_point = (backward_node < rb->node.oldest_reserve) ?
		((uint8_t*)backward_node + eaf_ringbuffer_node_cost(backward_node->token.size.size)) :
		(rb->config.cache);

	/* 第2步：计算连续的处于committed状态的节点能否容纳所需数据 */
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

	/* 第3步：检查是否具备overwrite的条件 */
	if (sum_size < node_size)
	{
		return NULL;
	}

	/* 第4步：执行overwrite */
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
* 更新oldest_reserve
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
	 * HEAD右侧的下一个可能的节点地址
	 * 若HEAD右侧存在节点，则其节点地址一定大于等于计算得出的节点地址
	 */
	ring_buffer_node_t* next_possible_node =
		(ring_buffer_node_t*)((uint8_t*)rb->node.HEAD
		+ eaf_ringbuffer_node_cost(rb->node.HEAD->token.size.size));

	/* 如果HEAD右侧存在其他节点，则尝试在两个节点之前形成token */
	if (rb->node.HEAD->chain_pos.p_forward > rb->node.HEAD)
	{
		/* 当HEAD右侧节点与HEAD之间的空隙足够时，可形成token */
		if ((size_t)((uint8_t*)rb->node.HEAD->chain_pos.p_forward
			- (uint8_t*)next_possible_node) >= node_size)
		{
			rb->counter.writing++;
			_ring_buffer_insert_new_node(rb, next_possible_node, data_len);
			_ring_buffer_reserve_update_oldest_reserve(rb, next_possible_node);
			return &next_possible_node->token;
		}

		/* 在允许的情况下进行overwrite */
		return (flags & eaf_ringbuffer_flag_overwrite) ?
			_ring_buffer_reserve_try_overwrite(rb, data_len, node_size) :
			NULL;
	}

	/* HEAD右侧不存在节点时，若HEAD右侧空间足够，则形成token */
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

	/* 更新计数器 */
	rb->counter.writing--;
	rb->counter.committed++;

	/* 修改节点状态 */
	node->state = stat_committed;

	return 0;
}

/**
* 删除node时，更新 chain_pos 与 chain_time
* @param node	待删除节点
*/
static void _ring_buffer_delete_node_update_chain(
	ring_buffer_node_t* node)
{
	/* 更新 chain_pos */
	node->chain_pos.p_backward->chain_pos.p_forward = node->chain_pos.p_forward;
	node->chain_pos.p_forward->chain_pos.p_backward =
		node->chain_pos.p_backward;

	/* 更新 chain_time */
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
	 * 当RingBuffer中仅包含一个节点时，重置RingBuffer
	 * 此处使用 p_forward 来判断是否仅包含一个节点，原因如下：
	 * 1. chain_pos.p_forward 总是指向环形链路中的下一节点，当等于自身时，环形链路中一定只有一个节点
	 * 2. 当 node 处于 cacheline 中时，p_forward 作为 node 的第一个元素，一定也处于 cacheline 中，可以避免访问内存
	 */
	if (node->chain_pos.p_forward == node)
	{
		_ring_buffer_reinit(rb);
		return;
	}

	/* 更新 chain_pos 和 chain_time */
	_ring_buffer_delete_node_update_chain(node);

	/* 更新 oldest_reserve */
	if (rb->node.oldest_reserve == node)
	{
		rb->node.oldest_reserve = node->chain_time.p_newer;
	}

	/*
	 * node 为 TAIL 节点时，更新 TAIL
	 */
	if (node->chain_time.p_older == NULL)
	{
		rb->node.TAIL = node->chain_time.p_newer;
		return;
	}

	/*
	 * node 为 HEAD 节点时，更新 HEAD
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
	/* 如果存在新的消费者，则应该失败 */
	if (node->chain_time.p_newer != NULL
		&& node->chain_time.p_newer->state == stat_reading)
	{
		return (flags & eaf_ringbuffer_flag_abandon) ?
			_ring_buffer_commit_for_consume_confirm(rb, node) : -1;
	}

	/* 修改计数器 */
	rb->counter.reading--;
	rb->counter.committed++;

	/* 修改状态 */
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
	/* 计算头部起始地点 */
	eaf_ringbuffer_t* handler = (void*)EAF_ALIGN(buffer, sizeof(void*));

	/* 计算为了对齐导致的空隙大小 */
	const size_t leading_align_size = (uint8_t*)handler - (uint8_t*)buffer;

	/** 检查剩余区域是否充足 */
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
