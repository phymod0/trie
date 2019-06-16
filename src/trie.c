#include <stdbool.h>

#include "trie.h"
#include "stack.h"


#define VALLOC(x, n) (x = malloc((n) * sizeof *(x)))
#define ALLOC(x) VALLOC(x, 1)
#define STR_FOREACH(str) for (; *str; ++str)


typedef struct trie_node {
	char* segment;
	struct trie_node *fchild, *next;
	void* value;
} trie_node_t;

typedef struct trie {
	trie_node_t* root;
	struct trie_ops* ops;
	size_t max_strlen_added;
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
	if (!ALLOC(trie)
	    || !ALLOC(root)
	    || !ALLOC(empty)
	    || !ALLOC(trie_ops))
		goto oom;

	empty[0] = '\0';
	root->segment = empty;
	root->fchild = root->next = NULL;
	root->value = NULL;
	trie->max_strlen_added = 0;
	trie->root = root;
	memcpy(trie_ops, ops, sizeof *trie_ops);
	trie->ops = trie_ops;

	return trie;

oom:
	free(trie);
	free(root);
	free(empty);
	free(trie_ops);
	return NULL;
}


static void node_recursive_free(trie_node_t* node, trieval_destructor_t dtor)
{
	if (!node)
		return;

	node_recursive_free(node->fchild, dtor);
	node_recursive_free(node->next, dtor);

	free(node->segment);
	if (dtor)
		dtor(node->value);

	free(node);
}


void trie_destroy(Trie* trie)
{
	if (!trie)
		return;

	node_recursive_free(trie->root, trie->ops->dtor);
	free(trie->ops);
	free(trie);
}


size_t trie_maxstrlen_added(Trie* trie)
{
	return trie->max_strlen_added;
}


static trie_node_t* node_create(char* segment, void* value)
{
	trie_node_t* node;
	if (!ALLOC(node) || !(node->segment = strdup(segment))) {
		free(node);
		return NULL;
	}
	node->fchild = node->next = NULL;
	node->value = value;
	return node;
}


static int node_split(trie_node_t* node, char* segptr)
{
	if (!*segptr)
		return 0;

	trie_node_t *child = node_create(segptr, node->value);
	if (!child)
		return -1;
	child->fchild = node->fchild;

	ptrdiff_t segment_len = segptr - node->segment;
	char* segment = strndup(node->segment, (size_t)segment_len);
	if (!segment) {
		free(child->segptr);
		free(child);
		return -1;
	}

	free(node->segment);
	node->segment = segment;
	node->fchild = child;
	node->value = NULL;
	return 0;
}


static char* add_strs(const char* str1, const char* str2)
{
	char* result = malloc(strlen(str1) + strlen(str2) + 1);
	if (!result)
		return NULL;
	strcat(result, str1);
	strcat(result, str2);
	return result;
}


static int node_merge(trie_node_t* node)
{
	trie_node_t* child = node->fchild;
	if (!child || child->next || node->value)
		return 0;

	char* segment = add_strs(node->segment, child->segment);
	if (!segment)
		/* FIXME: Merge failure violates the 'compactness' invariant */
		return -1;
	free(node->segment);
	node->segment = segment;
	node->fchild = child->fchild;
	node->value = child->value;

	free(child->segment);
	free(child);
	return 0;
}


static inline bool advance_and_check_mismatch(trie_node_t** node_p,
					      char** segptr_p, char c_key,
					      trie_node_t** node_prev_p)
{
	trie_node_t* node = *node_p;
	char *segptr = *segptr_p;

	if (segptr[0] && (++segptr)[0]) {
		*segptr_p = segptr;
		return segptr[0] != c_key;
	}

	trie_node_t* node_prev = node;
	for (trie_node_t* child = node->fchild; child; child = child->next) {
		if (child->segment[0] != c_key) {
			node_prev = child;
			continue;
		}
		*node_p = child;
		*segptr_p = child->segment;
		if (node_prev_p)
			*node_prev_p = node_prev;
		return false;
	}

	*segptr_p = segptr;
	return true;
}


static void find_mismatch(Trie* trie, const char* key, trie_node_t** node_p,
			  trie_node_t** pred_p, char** segptr_p,
			  const char** key_p)
{
	*node_p = trie->root;
	if (pred_p)
		*pred_p = NULL;
	*segptr_p = node->segment;

	char* key = *key_p;
	STR_FOREACH(key)
		if (advance_and_check_mismatch(node_p, segptr_p, *key, pred_p))
			break;
	*key_p = key;

	if (!key[0] && (*segptr_p)[0])
		/* The only case in which the mismatch comes next */
		++(*segptr_p);
}


int trie_insert(Trie* trie, const char* key, void* val)
{
	const char* key_old = key;

	trie_node_t* node;
	char *segptr;
	find_mismatch(trie, key, &node, NULL, &segptr, &key);

	trie_node_t* keybranch = NULL;
	if (*key && !(keybranch = node_create(key, val)))
		return -1;

	if (node_split(node, segptr) == -1) {
		if (keybranch) {
			free(keybranch->segment);
			free(keybranch);
		}
		return -1;
	}

	size_t key_strlen = strlen(key_old);
	if (key_strlen > trie->max_strlen_added)
		trie->max_strlen_added = key_strlen;

	if (keybranch) {
		keybranch->next = node->fchild;
		node->fchild = keybranch;
		return 0;
	}
	if (node->value && trie->ops->dtor)
		trie->ops->dtor(node->value);
	node->value = val;
	return 0;
}


int trie_delete(Trie* trie, const char* key)
{
	trie_node_t *node, *pred;
	char *segptr;
	find_mismatch(trie, key, &node, &pred, &segptr, &key);

	if (*key || *segptr)
		/* Not found */
		return 0;

	if (node->value && trie->ops->dtor)
		trie->ops->dtor(node->value);
	node->value = NULL;

	if (!pred)
		return 0;

	if (node->fchild)
		return node_merge(node);

	if (node == pred->fchild)
		pred->fchild = node->next;
	else
		pred->next = node->next;

	free(node->segptr);
	free(node);
	return 0;
}


void* trie_find(Trie* trie, const char* key)
{
	trie_node_t* node;
	char* segptr;
	find_mismatch(trie, key, &node, &segptr, &key);
	if (*key || *segptr)
		/* Not found */
		return NULL;
	return node->value;
}


static inline key_buffer_create(size_t max_keylen)
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

	trie_node_t* next = node->next;
	char* keyptr = stack_pop(keyptr_stack);
	if (next && (stack_push(node_stack, next) < 0
		     || stack_push(keyptr_stack, keyptr) < 0))
		goto oom;

	trie_node_t* fchild = node->fchild;
	char* keyend = key_add_segment(keyptr, node->segment, key, max_keylen);
	if (!keyend)
		return false;
	if (fchild && (stack_push(node_stack, fchild) < 0
		       || stack_push(keyptr_stack, keyend) < 0))
		goto oom;

	return (iter->value = node->value) ? true : false;

oom:
end_iterator:
	trie_iter_destroy(iter);
	*iter_p = NULL;
	return true;
}


static TrieIterator* trie_iter_create(const char* truncated_prefix,
				      trie_node_t* node, size_t max_keylen)
{
	TrieIterator* iter = NULL;
	char* keybuf = NULL;
	Stack *node_stack = NULL, *keyptr_stack = NULL;

	if (!ALLOC(iter))
		goto oom;

	char* fchild_keyptr;
	if (!(keybuf = key_buffer_create(max_keylen)))
		goto oom;
	if (!(fchild_keyptr = key_add_segment(keybuf, truncated_prefix, keybuf,
					      max_keylen)))
		goto return_empty_iterator;

	if (!(node_stack = stack_create(STACK_OPS_NULL))
	    || !(keyptr_stack = stack_create(STACK_OPS_NULL)))
		goto oom;
	if (node->fchild && (stack_push(node_stack, node->fchild) < 0
			     || stack_push(keyptr_stack, fchild_keyptr) < 0))
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
	find_mismatch(trie, key_prefix, &node, &segptr, &prefix_left);

	if (*prefix_left)
		/* Full prefix not found */
		return NULL;

	size_t key_pfxlen = strlen(key_prefix);
	ptrdiff_t seg_sfxlen = segptr - node->segment;
	char* truncated_prefix = strndup(key_prefix, key_pfxlen - seg_sfxlen);
	if (!truncated_prefix)
		return NULL;

	return trie_iter_create(truncated_prefix, node, max_keylen);
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
