#include <string.h>
#include "EAF/utils/map.h"

enum rb_color
{
	rb_red = 0x01,
	rb_blk = 0x02,
};

/**
* Init RBTree node
*
* @param node
*       RBTree node
*/
static void _rb_lite_init_node(eaf_map_node_t* node)
{
	node->color = rb_blk;
	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
}

/**
* Inserts node into the tree as if it were a regular binary tree
* using the algorithm described in _Introduction_To_Algorithms_
* by Cormen et al.  This function is only intended to be called
* by the RBTreeInsert function and not by the user
*
* If key in node conflict with data in tree, previous key and value will be replaced
*
* @param tree
*       the tree
* @param node
*       the node to be add
* @return
*       0: insert success, ready to re-balance
*       !0: replace success, no need to re-balance
*/
static int _rb_lite_insert_help(eaf_map_t* tree, eaf_map_node_t* node)
{
	int compval = 0;
	eaf_map_node_t* x = tree->root.left;
	eaf_map_node_t* y = &tree->root;
	eaf_map_node_t* nil = &tree->nil;
	node->right = nil;
	node->left = nil;

	while (x != nil)
	{
		y = x;
		compval = tree->cmp.cmp(x, node, tree->cmp.arg);

		if (compval == 0)
		{
			return -1;
		}

		x = compval > 0 ? x->left : x->right;
	}

	node->parent = y;

	if (y == &tree->root
		|| tree->cmp.cmp(y, node, tree->cmp.arg) > 0)
	{
		y->left = node;
	}
	else
	{
		y->right = node;
	}

	return 0;
}

/**
* Rotates as described in _Introduction_To_Algorithms by
* Cormen, Leiserson, Rivest (Chapter 14).  Basically this
* makes the parent of x be to the left of x, x the parent of
* its parent before the rotation and fixes other pointers
* accordingly.
*
* @param tree
*       the tree that it can access the appropriate root and nil pointers
* @param node
*       the node to rotate on
*/
static void _rb_lite_left_rotate(eaf_map_t* tree, eaf_map_node_t* node)
{
	eaf_map_node_t* y = node->right;
	eaf_map_node_t* nil = &tree->nil;

	node->right = y->left;

	if (y->left != nil)
	{
		y->left->parent = node;
	}

	y->parent = node->parent;

	if (node == node->parent->left)
	{
		node->parent->left = y;
	}
	else
	{
		node->parent->right = y;
	}

	y->left = node;
	node->parent = y;
}

/**
* Rotates as described in _Introduction_To_Algorithms by
* Cormen, Leiserson, Rivest (Chapter 14).  Basically this
* makes the parent of x be to the left of x, x the parent of
* its parent before the rotation and fixes other pointers
* accordingly.
*
* @param tree
*       the tree that it can access the appropriate root and nil pointers
* @param node
*       the node to rotate on
*/
static void _rb_lite_right_rotate(eaf_map_t* tree, eaf_map_node_t* node)
{
	eaf_map_node_t* x;
	eaf_map_node_t* nil = &tree->nil;
	x = node->left;
	node->left = x->right;

	if (nil != x->right)
	{
		x->right->parent = node;
	}

	x->parent = node->parent;

	if (node == node->parent->left)
	{
		node->parent->left = x;
	}
	else
	{
		node->parent->right = x;
	}

	x->right = node;
	node->parent = x;
}

/**
* This function returns the successor of x or NULL if no
* successor exists.
*
* @param tree
*       the tree contains node
* @param node
*       the node we want successor
*/
static eaf_map_node_t* _rb_lite_successor(eaf_map_t* tree, eaf_map_node_t* node)
{
	eaf_map_node_t* x = node;
	eaf_map_node_t* y;
	eaf_map_node_t* nil = &tree->nil;
	eaf_map_node_t* root = &tree->root;

	if (nil != (y = x->right))
	{
		while (y->left != nil)
		{
			y = y->left;
		}

		return y;
	}

	y = x->parent;
	while (x == y->right)
	{
		x = y;
		y = y->parent;
	}

	if (y == root)
	{
		return nil;
	}

	return y;
}

/**
* Performs rotations and changes colors to restore red-black
* properties after a node is deleted
*
* @param tree
*       the tree to fix
* @param node
*       node is the child of the spliced out node in rbtree_erase.
*/
static void _rb_lite_delete_fixup(eaf_map_t* tree, eaf_map_node_t* node)
{
	eaf_map_node_t* root = tree->root.left;
	eaf_map_node_t* w;

	while ((node->color == rb_blk) && (root != node))
	{
		if (node == node->parent->left)
		{
			w = node->parent->right;

			if (w->color == rb_red)
			{
				w->color = rb_blk;
				node->parent->color = rb_red;
				_rb_lite_left_rotate(tree, node->parent);
				w = node->parent->right;
			}

			if ((w->right->color == rb_blk) && (w->left->color == rb_blk))
			{
				w->color = rb_red;
				node = node->parent;
			}
			else
			{
				if (w->right->color == rb_blk)
				{
					w->left->color = rb_blk;
					w->color = rb_red;
					_rb_lite_right_rotate(tree, w);
					w = node->parent->right;
				}

				w->color = node->parent->color;
				node->parent->color = rb_blk;
				w->right->color = rb_blk;
				_rb_lite_left_rotate(tree, node->parent);
				node = root;
			}
		}
		else
		{
			w = node->parent->left;

			if (w->color == rb_red)
			{
				w->color = rb_blk;
				node->parent->color = rb_red;
				_rb_lite_right_rotate(tree, node->parent);
				w = node->parent->left;
			}

			if (w->right->color == rb_blk && w->left->color == rb_blk)
			{
				w->color = rb_red;
				node = node->parent;
			}
			else
			{
				if (w->left->color == rb_blk)
				{
					w->right->color = rb_blk;
					w->color = rb_red;
					_rb_lite_left_rotate(tree, w);
					w = node->parent->left;
				}

				w->color = node->parent->color;
				node->parent->color = rb_blk;
				w->left->color = rb_blk;
				_rb_lite_right_rotate(tree, node->parent);
				node = root;
			}
		}
	}

	node->color = rb_blk;
}

void eaf_map_init(eaf_map_t* tree, eaf_map_cmp_fn cmp, void* arg)
{
	eaf_map_node_t* tmp;

	memset(tree, 0, sizeof(eaf_map_t));
	tree->cmp.cmp = cmp;
	tree->cmp.arg = arg;

	_rb_lite_init_node(&tree->nil);
	tmp = &tree->nil;
	tmp->right = tmp;
	tmp->left = tmp;
	tmp->parent = tmp;
	tmp->color = rb_blk;

	_rb_lite_init_node(&tree->root);
	tmp = &tree->root;
	tmp->right = &tree->nil;
	tmp->left = &tree->nil;
	tmp->parent = &tree->nil;
	tmp->color = rb_blk;
}

int eaf_map_insert(eaf_map_t* tree, eaf_map_node_t* node)
{
	eaf_map_node_t* x;
	eaf_map_node_t* y;

	x = node;
	_rb_lite_init_node(x);

	if (_rb_lite_insert_help(tree, x) != 0)
	{// insert failed
		return -1;
	}
	else
	{// node added
		(tree->size)++;
	}

	//newNode = x;
	x->color = rb_red;

	while (x->parent->color == rb_red)
	{
		if (x->parent == x->parent->parent->left)
		{
			y = x->parent->parent->right;

			if (y->color == rb_red)
			{
				x->parent->color = rb_blk;
				y->color = rb_blk;
				x->parent->parent->color = rb_red;
				x = x->parent->parent;
			}
			else
			{
				if (x == x->parent->right)
				{
					x = x->parent;
					_rb_lite_left_rotate(tree, x);
				}

				x->parent->color = rb_blk;
				x->parent->parent->color = rb_red;
				_rb_lite_right_rotate(tree, x->parent->parent);
			}
		}
		else
		{
			y = x->parent->parent->left;

			if (y->color == rb_red)
			{
				x->parent->color = rb_blk;
				y->color = rb_blk;
				x->parent->parent->color = rb_red;
				x = x->parent->parent;
			}
			else
			{
				if (x == x->parent->left)
				{
					x = x->parent;
					_rb_lite_right_rotate(tree, x);
				}

				x->parent->color = rb_blk;
				x->parent->parent->color = rb_red;
				_rb_lite_left_rotate(tree, x->parent->parent);
			}
		}
	}

	tree->root.left->color = rb_blk;
	return 0;
}

void eaf_map_erase(eaf_map_t* tree, eaf_map_node_t* node)
{
	eaf_map_node_t* x;
	eaf_map_node_t* y;
	eaf_map_node_t* z = node;
	eaf_map_node_t* nil = &tree->nil;
	eaf_map_node_t* root = &tree->root;
	(tree->size)--;
	y = ((z->left == nil) || (z->right == nil)) ? z : _rb_lite_successor(tree, z);
	x = (y->left == nil) ? y->right : y->left;

	if (root == (x->parent = y->parent))
	{
		root->left = x;
	}
	else
	{
		if (y == y->parent->left)
		{
			y->parent->left = x;
		}
		else
		{
			y->parent->right = x;
		}
	}

	if (y != z)
	{
		if (y->color == rb_blk)
		{
			_rb_lite_delete_fixup(tree, x);
		}

		y->left = z->left;
		y->right = z->right;
		y->parent = z->parent;
		y->color = z->color;
		z->right->parent = y;
		z->left->parent = y;

		if (z == z->parent->left)
		{
			z->parent->left = y;
		}
		else
		{
			z->parent->right = y;
		}
	}
	else
	{
		if (y->color == rb_blk)
		{
			_rb_lite_delete_fixup(tree, x);
		}
	}
}

eaf_map_node_t* eaf_map_find(const eaf_map_t* tree, const eaf_map_node_t* key)
{
	eaf_map_node_t* x = tree->root.left;
	const eaf_map_node_t* nil = &tree->nil;

	int compval;
	while (x != nil)
	{
		compval = tree->cmp.cmp(x, key, tree->cmp.arg);
		if (compval == 0)
		{
			return x;
		}

		/**
		* 使用三元操作符，
		* 在较新版本的gcc下，可以通过一条指令完成跳转
		*/
		x = compval > 0 ? x->left : x->right;
	}

	return NULL;
}

eaf_map_node_t* eaf_map_find_lower(const eaf_map_t* tree, const eaf_map_node_t* key)
{
	int compval; // 对比值
	const eaf_map_node_t* nil = &tree->nil;
	eaf_map_node_t* currentNode = tree->root.left;	/* 迭代指针，指向当前节点 */

	const eaf_map_node_t* lowerNode = nil;
	/************************************************************************/
	/* 进行查找                                                              */
	/************************************************************************/
	while (currentNode != nil)
	{
		compval = tree->cmp.cmp(currentNode, key, tree->cmp.arg);

		if (compval > 0)
		{// 当前键值大于KEY，向左子节点移动
			currentNode = currentNode->left;
		}
		else if (compval < 0)
		{// 当前键值小于KEY，向右子节点移动
			lowerNode = currentNode;
			currentNode = currentNode->right;
		}
		else
		{// 键值相等，找到匹配项
			lowerNode = currentNode;
			break;
		}
	}

	return lowerNode != nil ? (eaf_map_node_t*)lowerNode : NULL;
}

eaf_map_node_t* eaf_map_find_upper(const eaf_map_t* tree, const eaf_map_node_t* key)
{
	int compval;	/* 对比值 */
	eaf_map_node_t* currentNode = tree->root.left;	/* 迭代指针，指向当前节点 */
	const eaf_map_node_t* nil = &tree->nil;	/* 空节点 */

	/************************************************************************/
	/* 进行查找                                                             */
	/************************************************************************/
	const eaf_map_node_t* upperNode = nil;	/* 较大值 */
	while (currentNode != nil)
	{
		compval = tree->cmp.cmp(currentNode, key, tree->cmp.arg);

		if (compval > 0)
		{// 当前键值大于KEY，向左子节点移动
			upperNode = currentNode;
			currentNode = currentNode->left;
		}
		else if (compval < 0)
		{// 当前键值小于KEY，向右子节点移动
			currentNode = currentNode->right;
		}
		else
		{// 键值相等，找到匹配项
			if (upperNode == nil)
			{
				upperNode = currentNode->right;
			}
			break;
		}
	};

	/************************************************************************/
	/* 找到匹配时，直接返回匹配项                                             */
	/************************************************************************/
	return upperNode != nil ? (eaf_map_node_t*)upperNode : NULL;
}

eaf_map_node_t* eaf_map_begin(const eaf_map_t* tree)
{
	const eaf_map_node_t* const nil = &tree->nil;
	const eaf_map_node_t* const root = &tree->root;
	const eaf_map_node_t* node = &tree->root;

	while (node->left != nil)
	{
		node = node->left;
	}

	if (node == nil || node == root)
	{
		return NULL;
	}

	return (eaf_map_node_t*)node;
}

eaf_map_node_t* eaf_map_next(const eaf_map_t* tree, const eaf_map_node_t* data)
{
	const eaf_map_node_t* const root = &tree->root;
	const eaf_map_node_t* const nil = &tree->nil;
	const eaf_map_node_t* node = data;

	if (node->right != nil)
	{
		node = node->right;

		while (node->left != nil)
		{
			node = node->left;
		}

		if (node == nil || node == root)
		{
			return NULL;
		}

		// finish
	}
	else
	{
		eaf_map_node_t* parent = node->parent;

		while (node == parent->right)
		{
			node = parent;
			parent = parent->parent;
		}

		if (parent != root)
		{
			node = parent;
		}
		else
		{
			node = parent;
			return NULL;
		}

		// finish
	}

	return (eaf_map_node_t*)node;
}

size_t eaf_map_size(const eaf_map_t* tree)
{
	return tree->size;
}
