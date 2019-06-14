#ifndef TRIE
#define TRIE


#include <stdlib.h>
#include <string.h>
#include <stddef.h>


/** Trie data structure. */
typedef struct {void* priv;} Trie;

/** Iterator type for iterating over (key, value) pairs in a subtrie. */
typedef struct {void* priv;} TrieIterator;

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
 * @param ops Set of trie operations
 * @returns Allocated trie structure
 */
Trie* trie_create(const struct trie_ops* ops);

/**
 * Destroy a trie.
 *
 * @param trie Trie returned by <code>trie_create</code>
 */
void trie_destroy(Trie* trie);

/**
 * Get the length of the longest key added in the trie.
 *
 * Sizes of keys that were previously added but do not exist are also counted.
 *
 * @param trie Trie context
 * @returns Size of the longest key
 */
size_t trie_maxstrlen_added(Trie* trie);

/**
 * Insert a key-value pair into a trie.
 *
 * Values inserted with a pre-existing key will replace the corresponding
 * pre-existing value and the pre-existing value will be destroyed.
 *
 * @param trie Trie context
 * @param key C-string of the key
 * @param val Pointer to the value
 * @returns 0 on success or -1 if out of memory
 */
int trie_insert(Trie* trie, const char* key, void* val);

/**
 * Delete a key-value pair from the trie.
 *
 * @param trie Trie context
 * @param key C-string of the key to remove
 * @returns 0 on success or -1 if out of memory
 */
/* XXX: Remove memory dependency */
int trie_delete(Trie* trie, const char* key);

/**
 * Find a value from the trie given it's key.
 *
 * @param trie Trie context
 * @param key C-string of the key the requested value was inserted with.
 * @returns Requested value or NULL if not found
 */
void* trie_find(Trie* trie, const char* key);

/**
 * Find all values from a trie given a common prefix of their keys.
 *
 * @param trie Trie context
 * @param key_prefix C-string prefixing all keys to consider
 * @param max_keylen Maximum key size to iterate over
 * @returns TrieIterator to iterate over all requested values or NULL
 * @see trie_iterator.h
 */
TrieIterator* trie_findall(Trie* trie, const char* key_prefix,
			   size_t max_keylen);

/**
 * Advance an iterator to the next valid (key, value) pair.
 *
 * An iterator may be invalidated after instantiation only if a key is deleted
 * which is prefixed by <code>key_prefix</code> and also prefixes the current
 * iterator key.
 *
 * @param it Pointer to valid iterator or NULL
 */
void trie_iter_step(TrieIterator** it);

/**
 * Get the key at the current iterator.
 *
 * @param it Current iterator
 * @returns Iterator key
 */
const char* trie_iter_getkey(TrieIterator* it);

/**
 * Get the value at the current iterator.
 *
 * @param it Current iterator
 * @returns Iterator value
 */
void* trie_iter_getval(TrieIterator* it);

/**
 * Destroy an iterator.
 *
 * @param it Iterator to destroy
 */
void trie_iter_destroy(TrieIterator* it);


#endif /* TRIE */
