/**
 * @file trie.h
 * @brief Methods for trie operations.
 */


#ifndef TRIE
#define TRIE


#include <stdlib.h>
#include <string.h>
#include <stddef.h>


/** Operations on trie values. */
struct TrieOps {
	void (*dtor)(void*); /**< Destructor for an inserted value. */
};


static inline struct TrieOps trie_makeops(void (*dtor)(void*))
{
	struct TrieOps ops;
	ops.dtor = dtor;
	return ops;
}


/** Free all inserted values with <code>free()</code>. */
#define TRIE_OPS_FREE trie_makeops(free)

/** Do not free inserted values. */
#define TRIE_OPS_NONE trie_makeops(NULL)


//////////////////////////////////////////////////////
//////////////////// TRIE SECTION ////////////////////
//////////////////////////////////////////////////////

/** Trie data structure. */
struct Trie;
#ifndef TRIE_FWD
#define TRIE_FWD
typedef struct Trie Trie;
#endif /* TRIE_FWD */


/**
 * Instantiate a trie.
 *
 * @param ops Set of trie value operations
 * @returns Allocated trie structure or NULL if out of memory
 */
Trie* trie_create(const struct TrieOps ops);

/**
 * Destroy a trie.
 *
 * @param trie Trie returned by <code>trie_create</code>
 */
void trie_destroy(Trie* trie);

/**
 * Get the length of the longest key added in the trie.
 *
 * Sizes of keys that were previously added but do not exist are counted.
 * Sizes of keys that were not added due to a failure are not counted.
 *
 * @param trie Trie context
 * @returns Size of the longest key
 */
size_t trie_maxkeylen_added(Trie* trie);

/**
 * Insert a key-value pair into a trie.
 *
 * Values inserted with a pre-existing key will replace the corresponding
 * pre-existing value and the pre-existing value will be destroyed.
 *
 * The insertion will fail either if the required amount of free memory is not
 * available or if <code>val</code> is NULL. In case of failure, the trie is
 * left unchanged.
 *
 * @param trie Trie context
 * @param key C-string of the key
 * @param val Non-null pointer to the value
 * @returns 0 on success or -1 on failure
 */
int trie_insert(Trie* trie, char* key, void* val);

/**
 * Delete a key from the trie.
 *
 * The deletion will fail if the required amount of free memory is not
 * available. In case of failure, the trie is left unchanged.
 *
 * @param trie Trie context
 * @param key C-string of the key to remove
 * @returns 0 on success or -1 on failure
 */
int trie_delete(Trie* trie, char* key);

/**
 * Find a value from the trie given it's key.
 *
 * @param trie Trie context
 * @param key C-string of the key the requested value was inserted with.
 * @returns Requested value or NULL if not found
 */
void* trie_find(Trie* trie, char* key);


//////////////////////////////////////////////////////////
//////////////////// ITERATOR SECTION ////////////////////
//////////////////////////////////////////////////////////

/** Iterator type for iterating over (key, value) pairs in a subtrie. */
struct TrieIterator;
#ifndef TRIE_ITER_FWD
#define TRIE_ITER_FWD
typedef struct TrieIterator TrieIterator;
#endif /* TRIE_ITER_FWD */


/**
 * Destroy an iterator.
 *
 * @param iter Iterator to destroy
 */
void trie_iter_destroy(TrieIterator* iter);

/**
 * Create an iterator to cover all keys with a given prefix and maximum size.
 *
 * NULL will be returned either if the amount of free memory required is not
 * available or if there are no keys to begin with subject to the given prefix
 * and length constraints.
 *
 * Keys will be enumerated in ascending lexicographic order, i.e.
 * <code>strcmp(key1, key2)</code> will be negative if <code>key1</code> is
 * enumerated before <code>key2</code>.
 *
 * Enumeration without a maximum length constraint can be done by passing the
 * return value of <code>trie_maxkeylen_added</code> in the third argument.
 *
 * @param trie Trie context
 * @param key_prefix C-string prefixing all keys to enumerate
 * @param max_len Upper bound on the lengths of the keys to enumerate
 * @returns Valid iterator or NULL
 */
TrieIterator* trie_findall(Trie* trie, const char* key_prefix, size_t max_len);

/**
 * Advance an iterator to the next valid (key, value) pair.
 *
 * Iterators are invalidated either if the iterator has ended or if the amount
 * of free memory required to proceed with the iteration is not available.
 * <code>*iter_p</code>, if invalidated, is set to NULL.
 *
 * Behavior of a valid iterator, after any trie modification, is undefined.
 *
 * @param iter_p Pointer to valid iterator or NULL
 */
void trie_iter_next(TrieIterator** iter_p);

/**
 * Get the key at the current iterator.
 *
 * The content of the string returned by this function is undefined if the
 * iterator is subsequently advanced.
 *
 * @param iter Current iterator
 * @returns Iterator key
 */
const char* trie_iter_getkey(const TrieIterator* iter);

/**
 * Get the value at the current iterator.
 *
 * @param iter Current iterator
 * @returns Iterator value or NULL if the iterator is invalid
 */
void* trie_iter_getval(const TrieIterator* iter);


#endif /* TRIE */
