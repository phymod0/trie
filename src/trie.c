#include <stdbool.h>

#include "trie.h"
#include "trie_iterator.h"


#define VALLOC(x, n) (x = malloc((n) * sizeof *(x)))
#define ALLOC(x) VALLOC(x, 1)
#define STR_FOREACH(c, str) for (char c = *str; c; c = *++str)


typedef struct trie_node {
	char* segment;
	struct trie_node *fchild, *next;
	void* value;
} trie_node_t;

typedef struct {
	trie_node_t* root;
} trie_priv_t;


Trie* trie_create(void)
{
	Trie* trie;
	trie_priv_t* priv = NULL;
	trie_node_t* root = NULL;
	char* empty;
	if (!ALLOC(trie) || !ALLOC(priv) || !ALLOC(root) || !ALLOC(empty)) {
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


void trie_destroy(Trie* trie, trieval_destructor_t dtor)
{
	trie_priv_t* trie_priv = trie->priv_data;

	node_recursive_free(trie_priv->root, dtor);
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


static trie_node_t* node_split(trie_node_t* node, char* segptr)
{
	if (!*segptr)
		return node;

	trie_node_t *child = node_create(segptr, node->value);
	if (!child)
		return NULL;
	child->fchild = node->fchild;

	ptrdiff_t segment_len = segptr - node->segment;
	char* segment = strndup(node->segment, (size_t)segment_len);
	if (!segment) {
		free(child->segptr);
		free(child);
		return NULL;
	}

	free(node->segment);
	node->segment = segment;
	node->fchild = child;
	node->value = NULL;
	return node;
}


static trie_node_t* node_keybranch(trie_node_t* node, char* keyptr, void* val)
{
	trie_node_t* child = node_create(keyptr, val);
}


int trie_insert(Trie* trie, const char* key, void* val)
{
	trie_priv_t* trie_priv = trie->priv_data;

	/*
	 * Maintain: keyptr, segptr, node
	 * For each nonzero char:
	 *	- Find next matching (node, segptr) in trie
	 *	- If none found (next is a non/deterministic mismatch) break
	 * If keyptr is ended:
	 *	- If segptr is ended, handle empty key insertion case
	 *		o Split at segptr without advancing
	 *	- If segptr is not ended, advance and split at segptr
	 * If keyptr is not ended:
	 *	- Split at segptr, instantiate and insert remaining key
	 * Splitting:
	 *	Only split at non-ended
	 * After splitting:
	 *	Add branched key at root if keyptr not ended
	 *	Insert/replace void* val at root or new branched child
	 * Next matching:
	 *	If not at segend, increment
	 *	If now at segend, search next, move to next if found
	 */

	/* Find first character mismatch */
	trie_node_t* node = trie_priv->root;
	char *segptr = node->segment;
	STR_FOREACH(c_key, key) {
		trie_node_t* child = node->fchild;
		if (*segptr)
			++segptr;
		if (*segptr && *segptr == c_key)
			continue;
		while (child && child->segment[0] != c_key)
			child = child->next;
		if (!*segptr && child) {
			node = child, segptr = child->segment;
			continue;
		}
		break;
	}
	segptr += !*key && *segptr; /* Advance to nearest mismatch */

	/* Create a node holding the remaining key if non-empty */
	trie_node_t* keybranch = NULL;
	if (*key && !(keybranch = node_create(key, val)))
		return -1;

	/* Split mismatching node at mismatching character */
	if (!(node = node_split(node, segptr))) {
		if (keybranch) {
			free(keybranch->segment);
			free(keybranch);
		}
		return -1;
	}

	/* Add the remaining key to the trie if it exists */
	if (keybranch) {
		keybranch->next = node->fchild;
		node->fchild = keybranch;
	} else {
		if (node->value)
			dtor(node->value);
		node->value = val;
	}
}


void trie_delete(Trie* trie, const char* key, trieval_destructor_t dtor)
{

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
