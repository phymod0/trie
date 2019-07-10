#include <stdbool.h>

#include "trie.h"
#include "stack.h"


#define VALLOC(x, type, n) (x = (type*)malloc((n) * sizeof *(x)))
#define ALLOC(x, type) VALLOC(x, type, 1)


typedef struct TrieNode {
	char* segment;
	size_t n_children;
	struct TrieNode* children;
	void* value;
} TrieNode;

struct Trie {
	TrieNode* root;
	struct TrieOps* ops;
	size_t max_keylen_added;
};
#ifndef TRIE_FWD
#define TRIE_FWD
typedef struct Trie Trie;
#endif /* TRIE_FWD */

struct TrieIterator {
	Stack *node_stack, *keyptr_stack;
	size_t max_keylen;
	char* key;
	void* value;
};
#ifndef TRIE_ITER_FWD
#define TRIE_ITER_FWD
typedef struct TrieIterator TrieIterator;
#endif /* TRIE_ITER_FWD */

typedef void (*destructor_t)(void*);


/* Utility functions */
static char* str_dup(const char*);
static char* str_n_dup(const char*, size_t);
static char* add_strs(const char*, const char*);
static inline char* key_buffer_create(size_t);
static inline char* segncpy(char*, const char*, size_t);
static inline char* key_add_segment(char*, const char*, char*, size_t);
static inline ptrdiff_t pflen_equal(const char*, const char*);
static void val_insert(TrieNode*, void*, destructor_t);

/* Search functions */
static inline TrieNode* leq_child(TrieNode*, char);
static void find_mismatch(Trie*, const char*, TrieNode**, TrieNode**, char**,
			  char**);

/* Deletion functions */
static void node_recursive_free(TrieNode*, destructor_t);
static void raw_node_destroy(TrieNode*);
static int node_delchild(TrieNode*, TrieNode*, destructor_t);

/* Addition functions */
static TrieNode* node_create(char*, void*);
static int node_split(TrieNode*, char*);
static int node_merge(TrieNode* node, destructor_t);
static int node_fork(TrieNode*, char*, TrieNode*);
static int node_addchild(TrieNode*, TrieNode*);
static int node_branch(TrieNode*, char*, TrieNode*);

/* Iterator functions */
static bool trie_iter_step(TrieIterator**);
static TrieIterator* trie_iter_create(const char*, TrieNode*, size_t);


Trie* trie_create(const struct TrieOps ops)
{
	Trie* trie = NULL;
	TrieNode* root = NULL;
	char* empty = NULL;
	struct TrieOps* trie_ops = NULL;
	TrieNode* children = NULL;
	if (!ALLOC(trie, Trie)
	    || !ALLOC(root, TrieNode)
	    || !VALLOC(empty, char, 1)
	    || !ALLOC(trie_ops, struct TrieOps)
	    || !VALLOC(children, TrieNode, 0))
		goto oom;

	empty[0] = '\0';
	root->segment = empty;
	root->n_children = 0;
	root->children = children;
	root->value = NULL;
	trie->max_keylen_added = 0;
	trie->root = root;
	memcpy(trie_ops, &ops, sizeof *trie_ops);
	trie->ops = trie_ops;

	return trie;

oom:
	free(trie);
	free(root);
	free(empty);
	free(trie_ops);
	free(children);
	return NULL;
}


void trie_destroy(Trie* trie)
{
	if (!trie)
		return;

	node_recursive_free(trie->root, trie->ops->dtor);
	free(trie->root);
	free(trie->ops);
	free(trie);
}


size_t trie_maxkeylen_added(Trie* trie)
{
	return trie->max_keylen_added;
}


int trie_insert(Trie* trie, char* key, void* val)
{
	char* segptr;
	TrieNode *new_child = NULL, *node;
	const size_t key_strlen = strlen(key);

	if (!val)
		return -1;

	find_mismatch(trie, key, &node, NULL, &segptr, &key);

	bool err;
	if (key[0]) {
		err = !(new_child = node_create(key, val))
		      || node_branch(node, segptr, new_child) < 0;
	} else {
		err = node_split(node, segptr) < 0;
		if (!err)
			val_insert(node, val, trie->ops->dtor);
	}
	if (err) {
		raw_node_destroy(new_child);
		return -1;
	}

	if (key_strlen > trie->max_keylen_added)
		trie->max_keylen_added = key_strlen;
	return 0;
}


int trie_delete(Trie* trie, char* key)
{
	TrieNode *node, *parent;
	char* segptr;
	find_mismatch(trie, key, &node, &parent, &segptr, &key);

	if (*key || *segptr)
		/* Not found */
		return 0;

	destructor_t dtor = trie->ops->dtor;

	if (node->n_children > 1 || !parent) {
		val_insert(node, NULL, dtor);
		return 0;
	}

	if (node->n_children == 1)
		return node_merge(node, dtor);

	if (node_delchild(parent, node, dtor) < 0)
		return -1;

	if (!parent->value && parent->n_children == 1 && parent != trie->root)
		return node_merge(parent, dtor);

	return 0;
}


void* trie_find(Trie* trie, char* key)
{
	TrieNode* node;
	char* segptr;
	find_mismatch(trie, key, &node, NULL, &segptr, &key);
	return *key || *segptr ? NULL : node->value;
}


void trie_iter_destroy(TrieIterator* iter)
{
	if (!iter)
		return;

	stack_destroy(iter->node_stack);
	stack_destroy(iter->keyptr_stack);
	free(iter->key);
	free(iter);
}


TrieIterator* trie_findall(Trie* trie, const char* key_prefix,
			   size_t max_keylen)
{
	TrieNode* node;
	char *segptr, *prefix_left;
	find_mismatch(trie, key_prefix, &node, NULL, &segptr, &prefix_left);

	if (*prefix_left)
		/* Full prefix not found */
		return NULL;

	char* trunc_prefix = add_strs(key_prefix, segptr);
	if (!trunc_prefix)
		return NULL;

	TrieIterator* iter = trie_iter_create(trunc_prefix, node, max_keylen);
	free(trunc_prefix);
	return iter;
}


void trie_iter_next(TrieIterator** iter_p)
{
	while (!trie_iter_step(iter_p));
}


const char* trie_iter_getkey(const TrieIterator* iter)
{
	return iter ? iter->key : NULL;
}


void* trie_iter_getval(const TrieIterator* iter)
{
	return iter ? iter->value : NULL;
}


static void node_recursive_free(TrieNode* node, destructor_t dtor)
{
	size_t n_children = node->n_children;
	TrieNode* children = node->children;
	for (size_t i = 0; i < n_children; ++i)
		node_recursive_free(&children[i], dtor);
	free(children);

	free(node->segment);
	if (dtor)
		dtor(node->value);
}


static void raw_node_destroy(TrieNode* node)
{
	if (!node)
		return;
	free(node->segment);
	free(node->children);
	free(node);
}


static char* str_dup(const char* str)
{
	size_t len = strlen(str);
	char* result;
	if (VALLOC(result, char, len + 1))
		memcpy(result, str, len + 1);
	return result;
}


static TrieNode* node_create(char* segment, void* value)
{
	char* seg = NULL;
	TrieNode *node = NULL, *children = NULL;
	if (!ALLOC(node, TrieNode)
	    || !(seg = str_dup(segment))
	    || !VALLOC(children, TrieNode, 0))
		goto oom;

	node->segment = seg;
	node->n_children = 0;
	node->children = children;
	node->value = value;
	return node;

oom:
	free(node);
	free(seg);
	free(children);
	return NULL;
}


static char* str_n_dup(const char* str, size_t n)
{
	size_t len = strlen(str), min = len < n ? len : n;
	char* result;
	if (VALLOC(result, char, min + 1)) {
		memcpy(result, str, min);
		result[min] = '\0';
	}
	return result;
}


static int node_split(TrieNode* node, char* at)
{
	if (!at[0])
		return 0;

	char* segment = NULL;
	TrieNode* child = NULL;
	size_t parent_seglen = (size_t)(at - node->segment);

	if (!(segment = str_n_dup(node->segment, parent_seglen))
	    || !(child = node_create(at, node->value)))
		goto oom;

	child->n_children = node->n_children;
	free(child->children);
	child->children = node->children;

	free(node->segment);
	node->segment = segment;
	node->n_children = 1;
	node->children = &child[0];
	node->value = NULL;
	return 0;

oom:
	free(segment);
	raw_node_destroy(child);
	return -1;
}


static char* add_strs(const char* str1, const char* str2)
{
	size_t len1 = strlen(str1), len2 = strlen(str2);
	char* result;
	if (!VALLOC(result, char, len1 + len2 + 1))
		return NULL;
	strcpy(result, str1);
	strcpy(result + len1, str2);
	return result;
}


static void val_insert(TrieNode* node, void* val, destructor_t dtor)
{
	if (node->value && dtor)
		dtor(node->value);
	node->value = val;
}


static int node_merge(TrieNode* node, destructor_t dtor)
{
	if (node->n_children != 1)
		return 0;
	TrieNode* child = node->children;

	char* new_segment = NULL;
	if (!(new_segment = add_strs(node->segment, child->segment)))
		goto oom;
	free(node->segment);
	free(child->segment);

	node->segment = new_segment;
	node->n_children = child->n_children;
	node->children = child->children;
	val_insert(node, child->value, dtor);

	free(child);
	return 0;

oom:
	free(new_segment);
	return -1;
}


static inline TrieNode* leq_child(TrieNode* node, char find)
{
	if (node->n_children == 0 || find < node->children[0].segment[0])
		return NULL;

	size_t s = 0, e = node->n_children;
	while (e - s > 1) {
		size_t m = (s + e) / 2;
		if (node->children[m].segment[0] <= find)
			s = m;
		else
			e = m;
	}
	return &node->children[s];
}


static inline ptrdiff_t pflen_equal(const char* of, const char* with)
{
	const char* of_old = of;
	while (*of++ == *with++ && of[-1]);
	return (of - 1) - of_old;
}


static void find_mismatch(Trie* trie, const char* key, TrieNode** node_p,
			  TrieNode** parent_p, char** seg_p, char** key_p)
{
	TrieNode *node = trie->root, *parent = NULL;
	char* seg = trie->root->segment;

	while (key[0] && !seg[0]) {
		TrieNode* child = leq_child(node, key[0]);
		if (!child || child->segment[0] != key[0])
			break;
		parent = node, node = child, seg = child->segment;
		ptrdiff_t pflen = pflen_equal(key, seg);
		key += pflen, seg += pflen;
	}

	*node_p = node;
	if (parent_p)
		*parent_p = parent;
	*seg_p = seg;
	*key_p = (char*)key;
}


static int node_fork(TrieNode* node, char* at, TrieNode* new_child)
{
	TrieNode *new_children = NULL, *split_child;

	if (!VALLOC(new_children, TrieNode, 2) || node_split(node, at) < 0)
		goto oom;

	split_child = &node->children[0];

	if (split_child->segment[0] < new_child->segment[0]) {
		new_children[0] = *split_child;
		new_children[1] = *new_child;
	} else {
		new_children[0] = *new_child;
		new_children[1] = *split_child;
	}
	node->children = new_children;
	node->n_children = 2;

	free(split_child);
	free(new_child);
	return 0;

oom:
	free(new_children);
	return -1;
}


static int node_addchild(TrieNode* node, TrieNode* new_child)
{
	char find = new_child->segment[0];
	ptrdiff_t ins;
	size_t n_children = node->n_children, sz1, sz2;
	TrieNode *children = node->children, *new_children = NULL, *leq;

	if (!VALLOC(new_children, TrieNode, n_children + 1))
		goto oom;

	leq = leq_child(node, find);
	ins = leq ? (leq - children) + 1 : 0;
	sz1 = ins * sizeof children[0];
	sz2 = (n_children - ins) * sizeof children[0];

	memcpy(new_children, children, sz1);
	new_children[ins] = *new_child;
	memcpy(&new_children[ins + 1], &children[ins], sz2);
	node->children = new_children;
	++node->n_children;

	free(children);
	free(new_child);
	return 0;

oom:
	free(new_children);
	return -1;
}


static int node_branch(TrieNode* node, char* at, TrieNode* child)
{
	return at[0] ? node_fork(node, at, child) : node_addchild(node, child);
}


static int node_delchild(TrieNode* node, TrieNode* child,
			 destructor_t dtor)
{
	ptrdiff_t del;
	size_t n_children = node->n_children, sz1, sz2;
	TrieNode *new_children, *children = node->children;
	char* child_segment = child->segment;
	TrieNode* child_children = child->children;
	void* child_value = child->value;

	if (!VALLOC(new_children, TrieNode, n_children - 1))
		goto oom;
	del = child - children;
	sz1 = del * sizeof children[0];
	sz2 = (n_children - del - 1) * sizeof children[0];

	memcpy(new_children, children, sz1);
	memcpy(&new_children[del], &children[del + 1], sz2);

	node->children = new_children;
	--node->n_children;
	free(children);

	free(child_segment);
	free(child_children);
	if (child_value && dtor)
		dtor(child_value);
	return 0;

oom:
	return -1;
}


static inline char* key_buffer_create(size_t max_keylen)
{
	char* buf;
	if (!VALLOC(buf, char, max_keylen + 1))
		return NULL;
	buf[0] = '\0';
	return buf;
}


static inline char* segncpy(char* dest, const char* src, size_t n)
{
	while (n-- && (*dest++ = *src++));
	return dest - 1;
}


static inline char* key_add_segment(char* key, const char* segment,
				    char* keybuf, size_t max_keylen)
{
	char* keybuf_end = keybuf + max_keylen + 1;
	ptrdiff_t keybuf_left = keybuf_end - key;
	char* key_end = segncpy(key, segment, (size_t)keybuf_left);
	return *key_end ? NULL : key_end;
}


static bool trie_iter_step(TrieIterator** iter_p)
{
	TrieIterator* iter = *iter_p;
	if (!iter)
		return true;

	TrieNode* node;
	char *keyptr, *keyend;
	Stack* node_stack = iter->node_stack;
	Stack* keyptr_stack = iter->keyptr_stack;
	const size_t max_keylen = iter->max_keylen;
	char* key = iter->key;

	if (stack_empty(node_stack))
		goto end_iterator;

	node = (TrieNode*)stack_pop(node_stack);
	keyptr = (char*)stack_pop(keyptr_stack);
	keyend = key_add_segment(keyptr, node->segment, key, max_keylen);
	if (!keyend)
		return false;

	for (size_t i = node->n_children; i != 0; --i)
		if (stack_push(node_stack, &node->children[i - 1]) < 0
		    || stack_push(keyptr_stack, keyend) < 0)
			goto oom;

	return (iter->value = node->value) ? true : false;

oom:
end_iterator:
	trie_iter_destroy(iter);
	*iter_p = NULL;
	return true;
}


static TrieIterator* trie_iter_create(const char* truncated_prefix,
				      TrieNode* node, size_t max_keylen)
{
	TrieIterator* iter = NULL;
	char *keybuf = NULL, *child_keyptr;
	Stack *node_stack = NULL, *keyptr_stack = NULL;

	if (!ALLOC(iter, TrieIterator))
		goto oom;

	if (!(keybuf = key_buffer_create(max_keylen)))
		goto oom;
	if (!(child_keyptr = key_add_segment(keybuf, truncated_prefix, keybuf,
					     max_keylen)))
		goto return_empty_iterator;

	if (!(node_stack = stack_create(STACK_OPS_NONE))
	    || !(keyptr_stack = stack_create(STACK_OPS_NONE)))
		goto oom;
	for (size_t i = node->n_children; i != 0; --i)
		if (stack_push(node_stack, &node->children[i - 1]) < 0
		    || stack_push(keyptr_stack, child_keyptr) < 0)
			goto oom;

	iter->node_stack = node_stack;
	iter->keyptr_stack = keyptr_stack;
	iter->max_keylen = max_keylen;
	iter->key = keybuf;
	iter->value = node->value;

	if (iter->value)
		return iter;

	while (!trie_iter_step(&iter))
		continue;
	return iter;

oom:
return_empty_iterator:
	free(iter);
	free(keybuf);
	stack_destroy(node_stack);
	stack_destroy(keyptr_stack);
	return NULL;
}


#undef ALLOC
#undef VALLOC
