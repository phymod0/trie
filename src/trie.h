#ifndef TRIE
#define TRIE


#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "trie_iterator.h"


/** Trie data structure. */
typedef struct {void* priv_data;} Trie;

/** Destructor type for a trie node value. */
typedef void (*trieval_destructor_t)(void*);

/** Trie operations */
struct trie_ops {
	trieval_destructor_t dtor;
};


/**
 * Instantiate a trie.
 *
 * <code>ops</code> will be duplicated and then used.
 *
 * @returns Allocated trie structure
 */
Trie* trie_create(const struct trie_ops* ops);

/**
 * Destroy a trie.
 *
 * @param trie Trie returned by <code>trie_create</code>
 * @param dtor Pointer to the destructor of the value to destroy
 */
void trie_destroy(Trie* trie);

/**
 * Insert a key-value pair into a trie.
 *
 * Values inserted with a pre-existing key will replace the corresponding
 * pre-existing value and the pre-existing value will be destroyed.
 *
 * @param key C-string of the key
 * @param val Pointer to the value
 * @returns 0 on success or -1 if out of memory
 */
int trie_insert(Trie* trie, const char* key, void* val);

/**
 * Delete a key-value pair from the trie.
 *
 * @param key C-string of the key to remove
 * @param dtor Pointer to the destructor of the value to destroy
 * @returns 0 on success or -1 if out of memory
 */
/* XXX: Remove memory dependency */
int trie_delete(Trie* trie, const char* key);

/**
 * Find a value from the trie given it's key.
 *
 * @param key C-string of the key the requested value was inserted with.
 * @returns Requested value or NULL if not found
 */
void* trie_find(Trie* trie, const char* key);

/**
 * Find all values from a trie given a common prefix of their keys.
 *
 * @param key_prefix C-string prefixing all keys to consider
 * @returns TrieIterator to iterate over all requested values
 * @see trie_iterator.h
 */
TrieIterator* trie_findall(Trie* trie, const char* key_prefix);


#endif /* TRIE */
