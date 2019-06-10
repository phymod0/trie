#ifndef TRIE
#define TRIE


#include "trie_iterator.h"


typedef struct {void* priv_data;} Trie;
typedef void (*trieval_destructor_t)(void*);


/**
 * Instantiate a trie.
 *
 * @returns Allocated trie structure
 */
Trie* trie_create(void);

/**
 * Destroy a trie.
 *
 * @param trie Trie returned by <code>trie_create</code>
 */
void trie_destroy(Trie* trie, trieval_destructor_t dtor);

/**
 * Insert a key-value pair into a trie.
 *
 * Values inserted with a pre-existing key will replace the corresponding
 * pre-existing value.
 *
 * @param key C-string of the key
 * @param val Pointer to the value
 * @returns 0 on success or -1 if out of memory
 */
int trie_insert(Trie* trie, const char* key, void* val);

/**
 * Delete a key-value pair from the trie.
 *
 * The destructor is defaulted to <code>free</code> when <code>dtor</code> is
 * <code>NULL</code>.
 *
 * @param key C-string of the key to remove
 * @param dtor Optional pointer to the destructor of the value to destroy
 */
void trie_delete(Trie* trie, const char* key, trieval_destructor_t dtor);

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
