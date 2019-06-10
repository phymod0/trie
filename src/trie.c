#include "trie.h"
#include "trie_iterator.h"


#define VALLOC(x, n) (x = malloc((n) * sizeof *(x)))
#define ALLOC(x) VALLOC(x, 1)


typedef struct trie_node {
	char* str_segment;
	struct trie_node *first_child, *next;
	void* value;
} trie_node_t;

typedef struct {
	trie_node_t* root;
} trie_priv_t;


Trie* trie_create(void)
{
	Trie* trie;
	trie_priv_t* trie_priv;
	if (!ALLOC(trie) || !ALLOC(trie_priv)) {
		free(trie);
		return NULL;
	}

	trie_priv->root = NULL;
	trie->priv_data = trie_priv;

	return trie;
}


static void node_recursive_free(trie_node_t* node, trieval_destructor_t dtor)
{
	if (!node)
		return;

	node_recursive_free(node->first_child, dtor);
	node_recursive_free(node->next, dtor);

	free(node->str_segment);
	(dtor ? dtor : free)(node->value);

	free(node);
}


void trie_destroy(Trie* trie, trieval_destructor_t dtor)
{
	trie_priv_t* trie_priv = trie->priv_data;

	node_recursive_free(trie_priv->root, dtor);
	free(trie_priv);
	free(trie);
}


int trie_insert(Trie* trie, const char* key, void* val)
{

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


#undef ALLOC
#undef VALLOC
