# trie
Memory-efficient trie library for ANSI C (C99).
Tries remain compact under all operations i.e. existing keys are always partitioned among the minimum required number of nodes.


## Testing
cd test && make check


## API
~~~c
struct trie_ops {
	void (*dtor)(void*);
};

//////////////////////////////////////////////////////
//////////////////// TRIE SECTION ////////////////////
//////////////////////////////////////////////////////

struct trie;
typedef struct trie Trie;

Trie* trie_create(const struct trie_ops* ops);
void trie_destroy(Trie* trie);
size_t trie_maxkeylen_added(Trie* trie);
int trie_insert(Trie* trie, char* key, void* val);
int trie_delete(Trie* trie, char* key);
void* trie_find(Trie* trie, char* key);

//////////////////////////////////////////////////////////
//////////////////// ITERATOR SECTION ////////////////////
//////////////////////////////////////////////////////////

struct trie_iter;
typedef struct trie_iter TrieIterator;

void trie_iter_destroy(TrieIterator* iter);
TrieIterator* trie_findall(Trie* trie, const char* key_prefix, size_t max_keylen);
void trie_iter_next(TrieIterator** iter_p);
const char* trie_iter_getkey(TrieIterator* iter);
void* trie_iter_getval(TrieIterator* iter);
~~~
Refer to src/trie.h or doc/html/index.html for the documentation
