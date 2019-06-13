#include <stdbool.h>

#include "trie.h"
#include "trie_iterator.h"


#define VALLOC(x, n) (x = malloc((n) * sizeof *(x)))
#define ALLOC(x) VALLOC(x, 1)
#define STR_FOREACH(str) for (; *str; ++str)


typedef struct trie_node {
	char* segment;
	struct trie_node *fchild, *next;
	void* value;
} trie_node_t;

typedef struct {
	trie_node_t* root;
	struct trie_ops* ops;
} trie_priv_t;


Trie* trie_create(const struct trie_ops* ops)
{
	Trie* trie;
	trie_priv_t* priv = NULL;
	trie_node_t* root = NULL;
	char* empty = NULL;
	if (!ALLOC(trie)
	    || !ALLOC(priv)
	    || !ALLOC(root)
	    || !ALLOC(empty)
	    || !ALLOC(trie->ops)) {
		free(empty);
		free(root);
		free(priv);
		free(trie);
		return NULL;
	}

	empty[0] = '\0';
	root->segment = empty;
	root->fchild = root->next = NULL;
	root->value = NULL;
	priv->root = root;
	memcpy(trie->ops, ops, sizeof *(trie->ops));
	trie->priv_data = priv;

	return trie;
}


static void node_recursive_free(trie_node_t* node, trieval_destructor_t dtor)
{
	if (!node)
		return;

	node_recursive_free(node->fchild, dtor);
	node_recursive_free(node->next, dtor);

	free(node->segment);
	dtor(node->value);

	free(node);
}


void trie_destroy(Trie* trie)
{
	trie_priv_t* trie_priv = trie->priv_data;

	node_recursive_free(trie_priv->root, trie_priv->ops->dtor);
	free(trie_priv);
	free(trie);
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


static void find_mismatch(trie_priv_t* trie, const char* key,
			  trie_node_t** node_p, char** segptr_p,
			  const char** key_p)
{
	trie_node_t* node = trie->root;
	char* segptr = node->segment;

	STR_FOREACH(key) {
		trie_node_t* child = node->fchild;
		if (*segptr)
			++segptr;
		if (*segptr && *segptr == *key)
			continue;
		while (child && child->segment[0] != *key)
			child = child->next;
		if (!*segptr && child) {
			node = child, segptr = child->segment;
			continue;
		}
		break;
	}
	if (!*key && *segptr)
		/* Only case in which the mismatch comes next */
		++segptr;

	*node_p = node;
	*segptr_p = segptr;
	*key_p = key;
}


static void find_mismatch_parent(trie_priv_t* trie, const char* key,
				 trie_node_t** node_p, trie_node_t** parent_p,
				 char** segptr_p, const char** key_p)
{
	trie_node_t *node = trie->root, *parent = NULL;
	char* segptr = node->segment;

	STR_FOREACH(key) {
		trie_node_t* child = node->fchild;
		if (*segptr)
			++segptr;
		if (*segptr && *segptr == *key)
			continue;
		while (child && child->segment[0] != *key)
			child = child->next;
		if (!*segptr && child) {
			parent = node, node = child, segptr = child->segment;
			continue;
		}
		break;
	}
	if (!*key && *segptr)
		/* Only case in which the mismatch comes next */
		++segptr;

	*node_p = node;
	*parent_p = parent;
	*segptr_p = segptr;
	*key_p = key;
}


int trie_insert(Trie* trie, const char* key, void* val)
{
	trie_priv_t* trie_priv = trie->priv_data;

	trie_node_t* node;
	char *segptr;
	find_mismatch(trie_priv, key, &node, &segptr, &key);

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

	if (keybranch) {
		keybranch->next = node->fchild;
		node->fchild = keybranch;
		return 0;
	}
	if (node->value)
		trie_priv->ops->dtor(node->value);
	node->value = val;
	return 0;
}


int trie_delete(Trie* trie, const char* key)
{
	trie_priv_t* trie_priv = trie->priv_data;

	trie_node_t *node, *parent;
	char *segptr;
	find_mismatch_parent(trie_priv, key, &node, &parent, &segptr, &key);

	if (*key || *segptr)
		/* Not found */
		return 0;

	if (node->value)
		trie_priv->ops->dtor(node->value);
	node->value = NULL;

	if (!parent)
		return 0;

	if (node->fchild)
		return node_merge(node);
	parent->fchild = node->next;
	free(node->segptr);
	free(node);
	return 0;
}


void* trie_find(Trie* trie, const char* key)
{

}


TrieIterator* trie_findall(Trie* trie, const char* key_prefix)
{

}


#undef STR_FOREACH
#undef ALLOC
#undef VALLOC
