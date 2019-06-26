#include <assert.h>
#include <stdbool.h>

#include "trie.h"
#include "stack.h"


#define VALLOC(x, n) (x = malloc((n) * sizeof *(x)))
#define ALLOC(x) VALLOC(x, 1)
#define STR_FOREACH(str) for (; *str; ++str)


typedef struct trie_node {
	char* segment;
	size_t n_children;
	struct trie_node* children;
	void* value;
} trie_node_t;

typedef struct trie {
	trie_node_t* root;
	struct trie_ops* ops;
	size_t max_keylen_added;
} Trie;

typedef struct trie_iter {
	Stack *node_stack, *keyptr_stack;
	size_t max_keylen;
	char* key;
	void* value;
} TrieIterator;


Trie* trie_create(const struct trie_ops* ops)
{
	Trie* trie = NULL;
	trie_node_t* root = NULL;
	char* empty = NULL;
	struct trie_ops* trie_ops = NULL;
	trie_node_t* children = NULL;
	if (!ALLOC(trie)
	    || !ALLOC(root)
	    || !VALLOC(empty, 1)
	    || !ALLOC(trie_ops)
	    || !VALLOC(children, 0))
		goto oom;

	empty[0] = '\0';
	root->segment = empty;
	root->n_children = 0;
	root->children = children;
	root->value = NULL;
	trie->max_keylen_added = 0;
	trie->root = root;
	memcpy(trie_ops, ops, sizeof *trie_ops);
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


static void node_recursive_free(trie_node_t* node, void (*dtor)(void*))
{
	size_t n_children = node->n_children;
	trie_node_t* children = node->children;
	for (size_t i = 0; i < n_children; ++i)
		node_recursive_free(&children[i], dtor);
	free(children);

	free(node->segment);
	if (dtor)
		dtor(node->value);
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


static void raw_node_destroy(trie_node_t* node)
{
	if (!node)
		return;
	free(node->segment);
	free(node->children);
	free(node);
}

static trie_node_t* node_create(char* segment, void* value)
{
	char* seg = NULL;
	trie_node_t *node = NULL, *children = NULL;
	if (!ALLOC(node) || !(seg = strdup(segment)) || !VALLOC(children, 0))
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

static int node_split(trie_node_t* node, char* at)
{
	if (!at[0])
		return 0;

	char* segment = NULL;
	trie_node_t *child = NULL;
	size_t parent_seglen = (size_t)(at - node->segment);

	if (!(segment = strndup(node->segment, parent_seglen))
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
	if (!VALLOC(result, len1 + len2 + 1))
		return NULL;
	strcpy(result, str1);
	strcpy(result + len1, str2);
	return result;
}

static void val_insert(trie_node_t* node, void* val, void (*dtor)(void*))
{
	if (node->value && dtor)
		dtor(node->value);
	node->value = val;
}

static int node_merge(trie_node_t* node, void (*dtor)(void*))
{
	if (node->n_children != 1)
		return 0;
	trie_node_t* child = node->children;

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

static inline trie_node_t* leq_child(trie_node_t* node, char find)
{
	/* TODO: Binary search */
	size_t n_children = node->n_children;
	for (size_t i = 0; i < n_children; ++i)
		if (node->children[i].segment[0] > find)
			return i > 0 ? &node->children[i - 1] : NULL;
	return n_children ? &node->children[n_children - 1] : NULL;
}

static inline bool advance_to_mismatch(trie_node_t** node_p, char** seg_p,
				       char expect, trie_node_t** parent_p)
{
	char *segptr = *seg_p;
	if (segptr[0] && (++segptr)[0]) {
		*seg_p = segptr;
		return segptr[0] != expect;
	}

	trie_node_t *node = *node_p, *child = leq_child(node, expect);
	if (!child || child->segment[0] != expect) {
		*seg_p = segptr;
		return true;
	}

	*node_p = child;
	*seg_p = child->segment;
	if (parent_p)
		*parent_p = node;
	return false;
}

static void find_mismatch(Trie* trie, const char* key, trie_node_t** node_p,
			  trie_node_t** parent_p, char** seg_p, char** key_p)
{
	*node_p = trie->root;
	if (parent_p)
		*parent_p = NULL;
	*seg_p = trie->root->segment;

	STR_FOREACH(key)
		if (advance_to_mismatch(node_p, seg_p, *key, parent_p))
			break;

	*key_p = (char*)key;

	if (!key[0] && (*seg_p)[0])
		/* The only case in which the mismatch comes next */
		++(*seg_p);
}

static int node_fork(trie_node_t* node, char* at, trie_node_t* new_child)
{
	trie_node_t* new_children = NULL;

	if (!VALLOC(new_children, 2) || node_split(node, at) == -1)
		goto oom;

	trie_node_t* split_child = &node->children[0];

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

static int node_addchild(trie_node_t* node, trie_node_t* new_child)
{
	char find = new_child->segment[0];
	size_t n_children = node->n_children;
	trie_node_t *children = node->children, *new_children = NULL;

	if (!VALLOC(new_children, n_children + 1))
		goto oom;

	trie_node_t* leq = leq_child(node, find);
	ptrdiff_t ins = leq ? (leq - children) + 1 : 0;
	size_t sz1 = ins * sizeof children[0];
	size_t sz2 = (n_children - ins) * sizeof children[0];

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

static int node_branch(trie_node_t* node, char* at, trie_node_t* child)
{
	return at[0] ? node_fork(node, at, child) : node_addchild(node, child);
}

int trie_insert(Trie* trie, char* key, void* val)
{
	char *segptr;
	trie_node_t *new_child = NULL, *node;
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


static int delete_child(trie_node_t* node, trie_node_t* child,
			void (*dtor)(void*))
{
	size_t n_children = node->n_children;
	trie_node_t *new_children, *children = node->children;
	char* child_segment = child->segment;
	trie_node_t* child_children = child->children;
	void* child_value = child->value;

	if (!VALLOC(new_children, n_children - 1))
		goto oom;
	ptrdiff_t del = child - children;
	size_t sz1 = del * sizeof children[0];
	size_t sz2 = (n_children - del - 1) * sizeof children[0];

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

/* TODO: Document that the trie is not affected on failures */
/* TODO: Brief comments on static functions and sections? (put in TODO file) */
/* TODO: Move static functions to the bottom */
int trie_delete(Trie* trie, char* key)
{
	trie_node_t *node, *parent;
	char *segptr;
	find_mismatch(trie, key, &node, &parent, &segptr, &key);

	if (*key || *segptr)
		/* Not found */
		return 0;

	void (*dtor)(void*) = trie->ops->dtor;

	if (node->n_children > 1 || !parent) {
		val_insert(node, NULL, dtor);
		return 0;
	}

	if (node->n_children == 1)
		return node_merge(node, dtor);

	if (delete_child(parent, node, dtor) == -1)
		return -1;

	if (!parent->value && parent->n_children == 1 && parent != trie->root)
		return node_merge(parent, dtor);

	return 0;
}

void* trie_find(Trie* trie, char* key)
{
	trie_node_t* node;
	char* segptr;
	find_mismatch(trie, key, &node, NULL, &segptr, &key);
	return *key || *segptr ? NULL : node->value;
}


static inline char* key_buffer_create(size_t max_keylen)
{
	char* buf;
	if (!VALLOC(buf, max_keylen + 1))
		return NULL;
	buf[0] = '\0';
	return buf;
}


/* Copy at most n bytes to dest up to the terminating 0 of src */
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


void trie_iter_destroy(TrieIterator* iter)
{
	if (!iter)
		return;

	stack_destroy(iter->node_stack);
	stack_destroy(iter->keyptr_stack);
	free(iter->key);
	free(iter);
}


static bool trie_iter_step(TrieIterator** iter_p)
{
	TrieIterator* iter = *iter_p;
	if (!iter)
		return true;

	Stack* node_stack = iter->node_stack;
	Stack* keyptr_stack = iter->keyptr_stack;
	const size_t max_keylen = iter->max_keylen;
	char* key = iter->key;

	if (stack_empty(node_stack))
		goto end_iterator;

	trie_node_t* node = stack_pop(node_stack);
	char* keyptr = stack_pop(keyptr_stack);
	char* keyend = key_add_segment(keyptr, node->segment, key, max_keylen);
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


/* TODO: Change == -1 to < 0 */
/* TODO: Per-section comments? */
static TrieIterator* trie_iter_create(const char* truncated_prefix,
				      trie_node_t* node, size_t max_keylen)
{
	TrieIterator* iter = NULL;
	char* keybuf = NULL;
	Stack *node_stack = NULL, *keyptr_stack = NULL;

	if (!ALLOC(iter))
		goto oom;

	char* child_keyptr;
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


TrieIterator* trie_findall(Trie* trie, const char* key_prefix,
			   size_t max_keylen)
{
	trie_node_t* node;
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


const char* trie_iter_getkey(TrieIterator* iter)
{
	return iter ? iter->key : NULL;
}


void* trie_iter_getval(TrieIterator* iter)
{
	return iter ? iter->value : NULL;
}


#undef STR_FOREACH
#undef ALLOC
#undef VALLOC
