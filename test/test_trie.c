#include "trie.h"
#include "trie.c"
#include "stack.c"

#include "ctest.h"

#include <stdio.h>


TEST_DEFINE(test_instantiation, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

	if (!trie) {
		test_check(res, "NULL on out-of-memory", true);
		return;
	}

	TrieNode* root = trie->root;
	bool root_proper = root && root->segment && root->segment[0] == '\0'
			   && root->n_children == 0 && !root->value;
	test_check(res, "Proper tree structure", root_proper);

	struct TrieOps* ops = trie->ops;
	bool ops_proper = ops && ops->dtor == free;
	test_check(res, "Proper trie operations", ops_proper);

	bool keylen_proper = trie->max_keylen_added == 0;
	test_check(res, "Proper initial key max length", keylen_proper);

	trie_destroy(trie);
}


static inline char rand_char(void)
{
	return (char)((rand() % 255) + 1);
}

static inline size_t gen_len_bw(size_t min, size_t max)
{
	return (size_t)((rand() % (max - min + 1)) + min);
}

static char* gen_rand_str(size_t len)
{
	char* arr = malloc(len + 1);
	for (size_t i=0; i<len; ++i)
		arr[i] = rand_char();
	arr[len] = '\0';
	return arr;
}

#if 0
size_t n_freed;

static void increment(void* junk __attribute__((__unused__)))
{
	++n_freed;
}
#endif

static __attribute_used__ void __verbose_free(void* ptr)
{
	printf("About to free: %p\n", ptr), fflush(stdout);
	free(ptr);
}

TEST_DEFINE(asan_test_destroy, res)
{
	TEST_AUTONAME(res);

#if 0
	Trie* trie = trie_create(
			&(struct TrieOps){
				.dtor = __verbose_free,
			}
		     );
#endif
	Trie* trie = trie_create(TRIE_OPS_FREE);
	Trie* trie2 = trie_create(TRIE_OPS_NONE);
#if 0
	Trie* trie2 = trie_create(&(struct TrieOps){
		.dtor = increment,
	});
#endif
	if (!trie || !trie2) {
		test_check(res, "Trie allocation failed", false);
		trie_destroy(trie);
		trie_destroy(trie2);
		return;
	}

	size_t t = gen_len_bw(100, 200);
	while (t --> 0) {
		char* seg = gen_rand_str(t);
		void *junk = malloc(10);
		trie_insert(trie, seg, junk);
		trie_insert(trie2, seg, junk);
		free(seg);
	}

	trie_destroy(trie);
	trie_destroy(trie2);
#if 0
	n_freed = 0;
	test_check(res, "Insert count matches free count", n_freed == T);
#endif
}


TEST_DEFINE(test_max_keylen, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);
	test_check(res, "Key length initially 0",
		   trie_maxkeylen_added(trie) == 0);

	bool consistent = true;
	size_t t = gen_len_bw(100, 200);
	size_t M = 0;
	while (t --> 0) {
		size_t len = gen_len_bw(100, 200);
		char* seg = gen_rand_str(len);
		M = len > M ? len : M;
		trie_insert(trie, seg, malloc(1));
		free(seg);
		if (trie_maxkeylen_added(trie) != M) {
			consistent = false;
			break;
		}
	}
	test_check(res, "Key length stays consistent", consistent);

	trie_destroy(trie);
}


static __attribute_used__ TrieNode* gen_singleton_wlen(TestResult* res, size_t keylen)
{
	char* seg = gen_rand_str(keylen);
	void* value = malloc(100);
	TrieNode* node = node_create(seg, value);
	if (!node)
		test_check(res, "Node allocation failed", false);
	free(seg);
	return node;
}


static __attribute_used__ TrieNode* gen_singleton(TestResult* res)
{
	char* seg = gen_rand_str(gen_len_bw(1, 10));
	void* value = malloc(100);
	TrieNode* node = node_create(seg, value);
	if (!node)
		test_check(res, "Node allocation failed", false);
	free(seg);
	return node;
}


static __attribute_used__ void singleton_free(TrieNode* node)
{
	if (!node)
		return;
	free(node->segment);
	free(node->value);
	free(node->children);
	free(node);
}


TEST_DEFINE(test_node_create, res)
{
	TEST_AUTONAME(res);

	TrieNode* node = gen_singleton(res);
	if (!node) {
		test_check(res, "Node is NULL on failure", true);
		return;
	}
	char* seg = str_dup(node->segment);
	void* value = node->value;
	test_check(res, "No initial children", node->n_children == 0);
	bool seg_match = node->segment && strcmp(node->segment, seg) == 0;
	bool val_match = node->value && memcmp(node->value, value, 100) == 0;
	test_check(res, "Segment and value match", seg_match && val_match);

	singleton_free(node);
	free(seg);
}


static __attribute_used__ bool test_add(char* str1, char* str2, char* strsum)
{
	while (*str1 && *strsum)
		if (*str1++ != *strsum++)
			return false;
	while (*str2 && *strsum)
		if (*str2++ != *strsum++)
			return false;
	return *str2 == *strsum;
}


TEST_DEFINE(test_insert, res)
{
#define KEY_INSERT(str1, str2) \
	(trie_insert(trie,_seg=add_strs((str1),(str2)),malloc(10)),free(_seg))

	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

	test_check(res, "Inserting a NULL value fails with -1",
		   trie_insert(trie, "", NULL) == -1);

	void* __v = malloc(1);
	trie_insert(trie, "", __v);
	test_check(res, "Empty key inserted", trie->root->value == __v);

	// node1->node2
	// node2->node4
	// node2->node5
	// node1->node3
	// node3->node6
	char* seg1 = gen_rand_str(0);
	char* seg2_1 = gen_rand_str(gen_len_bw(1, 10));
	seg2_1[0] = 'a';
	char* seg2_2 = gen_rand_str(gen_len_bw(1, 10));
	char* seg2 = add_strs(seg2_1, seg2_2);
	char* seg3 = gen_rand_str(gen_len_bw(1, 10));
	seg3[0] = 'z';
	char* seg4 = gen_rand_str(gen_len_bw(1, 10));
	seg4[0] = 'p';
	char* seg5 = gen_rand_str(gen_len_bw(1, 10));
	seg5[0] = 'q';
	char* seg6 = gen_rand_str(gen_len_bw(1, 10));

	char* _seg = NULL;
	KEY_INSERT(seg2, seg4);
	KEY_INSERT(seg2, seg5);
	KEY_INSERT(seg3, "");
	KEY_INSERT(seg3, seg6);

	TrieNode* node1 = trie ? trie->root : NULL;
	TrieNode* node2 = node1 && node1->n_children > 0 ?
		&node1->children[0] : NULL;
	TrieNode* node4 = node2 && node2->n_children > 0 ?
		&node2->children[0]: NULL;
	TrieNode* node5 = node2 && node2->n_children > 1 ?
		&node2->children[1]: NULL;
	TrieNode* node3 = node1 && node1->n_children > 1 ?
		&node1->children[1] : NULL;
	TrieNode* node6 = node3 && node3->n_children > 0 ?
		&node3->children[0] : NULL;
	test_check(res, "Trie structure complete",
		   node1 && node2 && node3 && node4 && node5 && node6);
	test_check(res, "Trie segments formed as expected",
		   node1 && strcmp(node1->segment, seg1) == 0
		   && node2 && strcmp(node2->segment, seg2) == 0
		   && node3 && strcmp(node3->segment, seg3) == 0
		   && node4 && strcmp(node4->segment, seg4) == 0
		   && node5 && strcmp(node5->segment, seg5) == 0
		   && node6 && strcmp(node6->segment, seg6) == 0);

	free(seg1);
	free(seg2_1);
	free(seg2_2);
	free(seg2);
	free(seg3);
	free(seg4);
	free(seg5);
	free(seg6);
	trie_destroy(trie);
#undef KEY_INSERT
}


static __attribute_used__ bool tries_equal(TrieNode* node1, TrieNode* node2)
{
	if (!node1 || !node2)
		return !node1 && !node2;
	if (strcmp(node1->segment, node2->segment) != 0)
		return false;
	if (node1->n_children != node2->n_children)
		return false;
	size_t n_children = node1->n_children;
	for (size_t i = 0; i < n_children; ++i)
		if (!tries_equal(&node1->children[i], &node2->children[i]))
			return false;
	return true;
}

static bool test_compactness_invariant(TrieNode* node)
{
	if (!node->value && node->n_children == 1)
		return false;
	for (size_t i = 0; i < node->n_children; ++i)
		if (!test_compactness_invariant(&node->children[i]))
			return false;
	return true;
}

static __attribute_used__ bool test_compact(Trie* t)
{
	if (t->root->n_children < 1)
		return true;
	return test_compactness_invariant(&t->root->children[0]);
}

TEST_DEFINE(test_delete, res)
{
	TEST_AUTONAME(res);

	typedef enum {
		PASS,
		INSERT,
		DELETE,
		BOTH,
	} instruction;

	typedef struct record {
		char* key;
		instruction ins;
	} record;

	size_t n_rec, N = rand() & 1 ? 5 : 50;
	record* records = malloc((n_rec=gen_len_bw(3, N)) * sizeof *records);

	for (size_t i=0; i<n_rec; ++i) {
		records[i].key = gen_rand_str(gen_len_bw(0, N));
		for (size_t j=0; j<i; ++j)
			if (!strcmp(records[i].key, records[j].key)) {
				free(records[i--].key);
				continue;
			}
	}

	bool empty_noaffect = true, add_delete = true, compact = true;
	for (size_t iters=0; iters<N; ++iters) {
		Trie* trie_a = trie_create(TRIE_OPS_FREE);
		Trie* trie_b = trie_create(TRIE_OPS_FREE);
		for (size_t i=0; i<n_rec; ++i)
			records[i].ins = rand() % 4;

		for (size_t i=0; i<n_rec; ++i) {
			char* key = records[i].key;
			switch (records[i].ins) {
			case INSERT:
				trie_insert(trie_a, key, malloc(10));
				compact = compact && test_compact(trie_a);
				// fallthrough
			case BOTH:
				trie_insert(trie_b, key, malloc(10));
				compact = compact && test_compact(trie_b);
				// fallthrough
			default:
				break;
			}
		}

		for (size_t i=0; i<n_rec; ++i) {
			char* key = records[i].key;
			switch (records[i].ins) {
			case DELETE:
				trie_delete(trie_a, key);
				compact = compact && test_compact(trie_a);
				// fallthrough
			case BOTH:
				trie_delete(trie_b, key);
				compact = compact && test_compact(trie_b);
				// fallthrough
			default:
				break;
			}
		}

		trie_delete(trie_a, "");
		compact = compact && test_compact(trie_a);
		trie_delete(trie_b, "");
		compact = compact && test_compact(trie_b);
		empty_noaffect = empty_noaffect
			&& trie_a->root && trie_a->root->segment[0] == '\0'
			&& trie_b->root && trie_b->root->segment[0] == '\0';

		add_delete = add_delete
			&& tries_equal(trie_a->root, trie_b->root);
		trie_destroy(trie_a);
		trie_destroy(trie_b);
	}
	test_check(res, "Trie stays the same under corresponding add/deletes",
		   add_delete);
	test_check(res, "Empty value deletion does not affect structure",
		   empty_noaffect);
	test_check(res, "Trie stays as compact as possible after an operation",
		   compact);

	for (size_t i=0; i<n_rec; ++i)
		free(records[i].key);
	free(records);
}


TEST_DEFINE(test_find, res)
{
	TEST_AUTONAME(res);

	typedef struct key_val {
		char* key;
		void* val;
		bool ins;
	} key_val_t;

	size_t n_kv = rand() & 1 ? gen_len_bw(1, 4) : gen_len_bw(1, 50);
	key_val_t* kv = malloc(n_kv * sizeof kv[0]);
	for (size_t i=0; i<n_kv; ++i) {
		kv[i].key = gen_rand_str(gen_len_bw(1, 100));
		for (size_t j=0; j<i; ++j) {
			if (strcmp(kv[j].key, kv[i].key) == 0) {
				free(kv[i--].key);
				break;
			}
		}
	}
	for (size_t i=0; i<n_kv; ++i)
		kv[i].val = malloc(1);

	bool false_neg = false, false_pos = false;
	int n_iters = (int)gen_len_bw(100, 200);
	for (int iter = 0; iter < n_iters; ++iter) {
		Trie* trie = trie_create(TRIE_OPS_NONE);
		for (size_t i=0; i<n_kv; ++i) {
			if (rand()&1) {
				kv[i].ins = false;
				continue;
			}
			kv[i].ins = true;
			trie_insert(trie, kv[i].key, kv[i].val);
		}
		for (size_t i=0; i<n_kv; ++i) {
			void* val = trie_find(trie, kv[i].key);
			if (kv[i].ins && val != kv[i].val)
				false_neg = true;
			if (!kv[i].ins && val)
				false_pos = true;
		}
		trie_destroy(trie);
	}
	test_check(res, "No false negatives when finding", !false_neg);
	test_check(res, "No false positives when finding", !false_pos);

	for (size_t i=0; i<n_kv; ++i) {
		free(kv[i].key);
		free(kv[i].val);
	}
	free(kv);
}


TEST_DEFINE(test_segncpy, res)
{
	TEST_AUTONAME(res);

	char *buf = key_buffer_create(15), *oldbuf = buf;

	bool concat = true;
	buf = segncpy(buf, "Hello ", 100);
	concat = concat && strcmp(oldbuf, "Hello ") == 0;
	buf = segncpy(buf, "world!", 100);
	concat = concat && strcmp(oldbuf, "Hello world!") == 0;
	oldbuf[15] = '\0';
	buf = segncpy(buf, "Additional text", 3);
	bool partial_concat = strcmp(oldbuf, "Hello world!Add") == 0;

	test_check(res, "Normal concat correct", concat);
	test_check(res, "Partial concat correct", partial_concat);

	free(oldbuf);
}


TEST_DEFINE(test_key_add_segment, res)
{
	TEST_AUTONAME(res);

	char *buf = key_buffer_create(15), *oldbuf = buf;

	bool concat = true;
	buf = key_add_segment(buf, "Hello ", oldbuf, 15);
	concat = concat && strcmp(oldbuf, "Hello ") == 0;
	buf = key_add_segment(buf, "world!", oldbuf, 15);
	concat = concat && strcmp(oldbuf, "Hello world!") == 0;
	char* invalid_buf = key_add_segment(buf, "Additional", oldbuf, 15);
	char* valid_buf = key_add_segment(buf, "Add", oldbuf, 15);
	bool oob_concat = !invalid_buf;
	bool last_concat = strcmp(oldbuf, "Hello world!Add") == 0 && valid_buf
			     && oldbuf[15] == '\0' && valid_buf == &oldbuf[15];

	test_check(res, "Middle concat correct", concat);
	test_check(res, "NULL returned on out-of-bounds concat", oob_concat);
	test_check(res, "Last concat correct", last_concat);

	free(oldbuf);
}


TEST_DEFINE(asan_test_iter_destroy, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);
	size_t n_iters = gen_len_bw(100, 200);
	char* pref = NULL;
	for (size_t i=0; i<n_iters; ++i) {
		char* key = gen_rand_str(gen_len_bw(10, 100));
		trie_insert(trie, key, malloc(1));
		if (!pref)
			pref = key, pref[2] = '\0';
		else
			free(key);
	}

	TrieIterator* iter = trie_findall(trie, pref, 150);
	trie_iter_destroy(iter);

	trie_destroy(trie);
	free(pref);
}


static bool is_prefix(const char* prefix, const char* str)
{
	for (; prefix[0] && str[0]; ++prefix, ++str)
		if (prefix[0] != str[0])
			return false;
	return !prefix[0];
}

static bool lexical_lt(const char* str1, const char* str2)
{
	return str1 ? strcmp(str1, str2) < 0 : true;
}

TEST_DEFINE(test_iterator, res)
{
	TEST_AUTONAME(res);

	typedef struct key_val {
		char* key;
		void* val;
		bool ins;
	} key_val_t;

	Trie* trie = trie_create(TRIE_OPS_FREE);

	size_t N = rand()&1 ? 10 : 100, n_kv = gen_len_bw(100, 200),
	       pf_len = rand()&1 ? 0 : 3, max_keylen = gen_len_bw(N/4, 3*N/4),
	       n_findable = 0, n_found = 0;
	key_val_t* kv = malloc(n_kv * sizeof kv[0]);
	char* prf = NULL;
	for (size_t i=0; i<n_kv; ++i) {
		kv[i].key = gen_rand_str(gen_len_bw(10, N));
		kv[i].val = malloc(1);
		if (!prf) {
			prf = str_dup(kv[i].key), prf[pf_len] = 0;
			continue;
		}
		for (size_t j=0; j<i; ++j) {
			if (strcmp(kv[j].key, kv[i].key) == 0) {
				free(kv[i].key);
				free(kv[i].val);
				--i;
				break;
			}
		}
	}
	for (size_t i=0; i<n_kv; ++i) {
		if ((kv[i].ins = rand()&1)) {
			trie_insert(trie, kv[i].key, kv[i].val);
			if (is_prefix(prf, kv[i].key)
			    && strlen(kv[i].key) <= max_keylen)
				++n_findable;
		} else free(kv[i].val);
	}

	bool sorted = true, complete = true, sound = true, bounded = true,
	     prefixed = true, val_correct = true;
	TrieIterator* iter = trie_findall(trie, prf, max_keylen);
	const char* key = NULL;
	while (iter) {
		char* key_prev = key ? str_dup(key) : NULL;
		const char* key = trie_iter_getkey(iter);
		void* val = trie_iter_getval(iter);
		size_t i;
		for (i=0; i<n_kv; ++i) {
			if (strcmp(kv[i].key, key) == 0)
				break;
		}
		if (i == n_kv) {
			sound = false;
			trie_iter_next(&iter);
			free(key_prev);
			continue;
		}
		n_found += is_prefix(prf, key) && strlen(key) <= max_keylen;
		sorted = sorted && lexical_lt(key_prev, key);
		bounded = bounded && strlen(key) <= max_keylen;
		prefixed = prefixed && is_prefix(prf, key);
		val_correct = val_correct && val == kv[i].val;

		trie_iter_next(&iter);
		free(key_prev);
	}
	complete = n_findable == n_found;
	sound = sound && bounded && prefixed;

	test_check(res, "Iterated over keys in ascending order", sorted);
	test_check(res, "Did not iterate over out-of-bound keys", bounded);
	test_check(res, "Only covered keys with the given prefix", prefixed);
	test_check(res, "Value seen matched the value inserted", val_correct);
	test_check(res, "Iteration was complete", complete);
	test_check(res, "Iteration was sound", sound);

	trie_iter_destroy(iter);
	trie_destroy(trie);
	for (size_t i=0; i<n_kv; ++i)
		free(kv[i].key);
	free(kv);
	free(prf);
}


TEST_START
(
	test_instantiation,
	asan_test_destroy,
	test_max_keylen,
	test_node_create,
	test_insert,
	test_delete,
	test_find,
	test_segncpy,
	test_key_add_segment,
	asan_test_iter_destroy,
	test_iterator,
)
