# trie
Strictly conformant C/C++ trie library.
Tries remain compact under all operations i.e. existing strings are always partitioned among the minimum required number of nodes.
Wrapper class for C++11 provided in src/cpp11_wrapper/


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
int trie_insert(Trie* trie, char* key, void* val);
void* trie_find(Trie* trie, char* key);
int trie_delete(Trie* trie, char* key);
void trie_destroy(Trie* trie);
size_t trie_maxkeylen_added(Trie* trie);

//////////////////////////////////////////////////////////
//////////////////// ITERATOR SECTION ////////////////////
//////////////////////////////////////////////////////////

struct trie_iter;
typedef struct trie_iter TrieIterator;

TrieIterator* trie_findall(Trie* trie, const char* key_prefix, size_t max_len);
const char* trie_iter_getkey(const TrieIterator* iter);
void* trie_iter_getval(const TrieIterator* iter);
void trie_iter_next(TrieIterator** iter_p);
void trie_iter_destroy(TrieIterator* iter);
~~~
Refer to src/trie.h or doc/html/index.html for the documentation


## Testing
`cd test && make check`
