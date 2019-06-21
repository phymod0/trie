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

	trie_node_t* root = trie->root;
	bool root_proper = root && root->segment && root->segment[0] == '\0'
			   && !root->fchild && !root->next && !root->value;
	test_check(res, "Proper tree structure", root_proper);

	struct trie_ops* ops = trie->ops;
	bool ops_proper = ops && ops->dtor == free;
	test_check(res, "Proper trie operations", ops_proper);

	bool keylen_proper = trie->max_keylen_added == 0;
	test_check(res, "Proper initial key max length", keylen_proper);

	trie_destroy(trie);
}


static inline char rand_char(void)
{
	/* TODO: Remove this line when testing is complete */
	return (char)((rand() % 26) + 'a');
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

TEST_DEFINE(vg_test_destroy, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);
	Trie* trie2 = trie_create(TRIE_OPS_NONE);
#if 0
	Trie* trie2 = trie_create(&(struct trie_ops){
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
		trie_insert(trie, seg, NULL);
		free(seg);
		if (trie_maxkeylen_added(trie) != M) {
			consistent = false;
			break;
		}
	}
	test_check(res, "Key length stays consistent", consistent);

	trie_destroy(trie);
}


static trie_node_t* gen_singleton_wlen(test_result_t* res, size_t keylen)
{
	char* seg = gen_rand_str(keylen);
	void* value = malloc(100);
	trie_node_t* node = node_create(seg, value);
	if (!node)
		test_check(res, "Node allocation failed", false);
	free(seg);
	return node;
}


static trie_node_t* gen_singleton(test_result_t* res)
{
	char* seg = gen_rand_str(gen_len_bw(1, 10));
	void* value = malloc(100);
	trie_node_t* node = node_create(seg, value);
	if (!node)
		test_check(res, "Node allocation failed", false);
	free(seg);
	return node;
}


static void singleton_free(trie_node_t* node)
{
	if (!node)
		return;
	free(node->segment);
	free(node->value);
	free(node);
}


TEST_DEFINE(test_node_create, res)
{
	TEST_AUTONAME(res);

	trie_node_t* node = gen_singleton(res);
	if (!node) {
		test_check(res, "Node is NULL on failure", true);
		return;
	}
	char* seg = strdup(node->segment);
	void* value = node->value;
	test_check(res, "Pointers are initially NULL", !node->fchild &&
		   !node->next);
	bool seg_match = node->segment && strcmp(node->segment, seg) == 0;
	bool val_match = node->value && memcmp(node->value, value, 100) == 0;
	test_check(res, "Segment and value match", seg_match && val_match);

	singleton_free(node);
	free(seg);
}


static bool test_add(char* str1, char* str2, char* strsum)
{
	while (*str1 && *strsum)
		if (*str1++ != *strsum++)
			return false;
	while (*str2 && *strsum)
		if (*str2++ != *strsum++)
			return false;
	return *str2 == *strsum;
}

TEST_DEFINE(test_node_split, res)
{
	TEST_AUTONAME(res);

	bool result = true;
	for (size_t split=0; split<=100; ++split) {
		bool rchild = rand() & 1 ? true : false;
		bool rnext = rand() & 1 ? true : false;
		trie_node_t* node = gen_singleton_wlen(res, 100);
		trie_node_t* child = rchild ? gen_singleton(res) : NULL;
		trie_node_t* next = rnext ? gen_singleton(res) : NULL;
		if (!node || (rchild && !child) || (next && !next)) {
			singleton_free(node);
			singleton_free(child);
			singleton_free(next);
			return;
		}
		node->fchild = child;
		node->next = next;
		char* seg = strdup(node->segment);
		void* value = node->value;

		if (node_split(node, node->segment + split) < 0) {
			test_check(res, "Returns -1 on failure", true);
			singleton_free(node);
			singleton_free(child);
			singleton_free(next);
			free(seg);
			return;
		}

		if (split < 100) {
			result = result && node->next == next;
			result = result && node->value == NULL;
			result = result && node->fchild;
			result = result && !node->fchild->next;
			result = result && node->fchild->fchild == child;
			result = result && node->fchild->value == value;
			result = result && strlen(node->segment) == split;
			result = result && test_add(node->segment,
						    node->fchild->segment,
						    seg);
		} else if (split == 100) {
			result = result && node->fchild == child;
			result = result && node->next == next;
		}

		singleton_free(node);
		singleton_free(child);
		singleton_free(next);
		free(seg);

		if (!result)
			break;
	}
	test_check(res, "Correct splitting", result);
}


TEST_DEFINE(test_add_strs, res)
{
	TEST_AUTONAME(res);

	bool correct = true;
	size_t len = gen_len_bw(1000, 2000);
	while (len --> 0) {
		char* str1 = gen_rand_str(gen_len_bw(0, 10));
		char* str2 = gen_rand_str(gen_len_bw(0, 10));
		char* strsum = add_strs(str1, str2);
		correct = correct && test_add(str1, str2, strsum);
		free(strsum);
	}
	test_check(res, "Strings concatenating under addition", correct);
}


TEST_DEFINE(test_node_merge, res)
{
	TEST_AUTONAME(res);

	bool fchild_result = true;
	bool next_result = true;
	bool segment_result = true;
	bool value_result = true;
	for (size_t i=1; i<100; ++i) {
		trie_node_t* node = gen_singleton_wlen(res, 100);
		if (!node)
			return;
		char* __s = strdup(node->segment);
		void* __v = node->value;
		trie_node_t* child = node->fchild;
		trie_node_t* next = node->next;

		if (node_split(node, node->segment + i) == -1) {
			test_check(res, "Node splitting failed", false);
			singleton_free(node);
			free(__s);
			return;
		}

		if (node_merge(node) == -1) {
			test_check(res, "Merge fails with -1", true);
			singleton_free(node);
			free(__s);
			return;
		}

		if (!node) {
			fchild_result = false;
			next_result = false;
			segment_result = false;
			value_result = false;
			singleton_free(node);
			free(__s);
			break;
		}
		fchild_result = fchild_result && node->fchild == child;
		next_result = next_result && node->next == next;
		segment_result = segment_result && node->segment;
		value_result = value_result && node->value;
		segment_result = segment_result && !strcmp(node->segment, __s);
		value_result = value_result && node->value == __v;

		singleton_free(node);
		free(__s);
	}
	test_check(res, "Node value correct after merge", value_result);
	test_check(res, "Node segment correct after merge", segment_result);
	test_check(res, "Node next correct after merge", next_result);
	test_check(res, "Node child correct after merge", fchild_result);
}


#if 0
TEST_DEFINE(test_mischeck_incr, res)
{
	TEST_AUTONAME(res);

	while (1) {
		trie_node_t* root = gen_singleton_wlen(res, gen_len_bw(0, 4));
		trie_node_t* child1 = gen_singleton(res);
		trie_node_t* child2 = gen_singleton(res);
		trie_node_t* child3 = gen_singleton(res);
		if (!root || !child1 || !child2 || !child3)
			goto _return;
		char c1 = child1->segment[0];
		char c2 = child2->segment[0];
		char c3 = child3->segment[0];
		if (c1 <= (char)1 || c1 > c2 || c2 > c3 || *seg == CHAR_MAX)
			goto _retry;
		root->fchild = child1;
		child1->next = child2;
		child2->next = child3;

		trie_node_t* iter = root;
		char* seg = root->segment;
		trie_node_t* prev = NULL;
		/* Test midsegment mismatch */
		iter = root, prev = NULL;
		seg = root->segment + gen_len_bw(0, strlen(root->segment)-1);
		bool ret =
			advance_and_check_mismatch(&iter, &seg, *seg+1, &prev);
		test_check(res, "Returns false on mismatch", !ret);

		/* Check full match */
		iter = root, seg = root->segment, prev = NULL;
		trie_node_t* iter = root;
		char* seg = root->segment;
		trie_node_t* prev = NULL;
		for (; *seg; ++seg) {
			advance_and_check_mismatch(&iter, &seg, *seg);
		}

_return:
		singleton_free(root);
		singleton_free(child1);
		singleton_free(child2);
		singleton_free(child3);
		return;
_retry:
		singleton_free(root);
		singleton_free(child1);
		singleton_free(child2);
		singleton_free(child3);
	}
}


TEST_DEFINE(test_mischeck, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

	/* Continue below */

	trie_destroy(trie);
}
#endif


TEST_DEFINE(test_add_keybranch, res)
{
	TEST_AUTONAME(res);

	size_t b = gen_len_bw(1, 3);
	trie_node_t* node = gen_singleton(res);
	trie_node_t* child1 = gen_singleton(res);
	trie_node_t* child2 = gen_singleton(res);
	trie_node_t* child_add = gen_singleton(res);
	if (!node || !child1 || !child2 || !child_add) {
		singleton_free(node);
		singleton_free(child1);
		singleton_free(child2);
		singleton_free(child_add);
		return;
	}
	child1->segment[0] = 'f';
	child2->segment[0] = 'q';
	while (1) {
		char c = child_add->segment[0] = (char)(rand() & 0xFF);
		if (c != 'f' && c != 'q')
			break;
	}
	if (b > 1)
		node->fchild = child1;
	if (b > 2)
		child1->next = child2;

	bool sorted = true, child_exists = false;
	add_keybranch(node, child_add);
	for (trie_node_t* i = node->fchild; i; i = i->next, --b) {
		if (i->next && i->next->segment[0] <= i->segment[0])
			sorted = false;
		if (i == child_add)
			child_exists = true;
	}
	test_check(res, "One additional node added", b == 0);
	test_check(res, "Children sorted after addition", sorted);
	test_check(res, "Added child exists", child_exists);
}


TEST_DEFINE(test_insert, res)
{
#define KEY_INSERT(str1, str2) \
	(trie_insert(trie,_seg=add_strs((str1),(str2)),malloc(10)),free(_seg))

	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

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

	trie_node_t* node1 = trie ? trie->root : NULL;
	trie_node_t* node2 = node1 ? node1->fchild : NULL;
	trie_node_t* node4 = node2 ? node2->fchild : NULL;
	trie_node_t* node5 = node4 ? node4->next : NULL;
	trie_node_t* node3 = node2 ? node2->next : NULL;
	trie_node_t* node6 = node3 ? node3->fchild : NULL;
	test_check(res, "Trie structure complete",
		   node1 && node2 && node3 && node4 && node5 && node6);
	test_check(res, "Trie segments formed as expected",
		   node1 && strcmp(node1->segment, seg1) == 0
		   && node2 && strcmp(node2->segment, seg2) == 0
		   && node3 && strcmp(node3->segment, seg3) == 0
		   && node4 && strcmp(node4->segment, seg4) == 0
		   && node5 && strcmp(node5->segment, seg5) == 0
		   && node6 && strcmp(node6->segment, seg6) == 0);
	test_check(res, "Trie structure does not contain bad links",
		   (!node1 || node1->next == NULL)
		   && (!node3 || node3->next == NULL)
		   && (!node4 || node4->fchild == NULL)
		   && (!node5 || node5->next == NULL)
		   && (!node5 || node5->fchild == NULL)
		   && (!node6 || node6->next == NULL)
		   && (!node6 || node6->fchild == NULL));

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


static bool tries_equal(trie_node_t* node1, trie_node_t* node2)
{
	if (!node1 || !node2)
		return !node1 && !node2;
	if (strcmp(node1->segment, node2->segment) != 0)
		return false;
	return tries_equal(node1->fchild, node2->fchild)
		&& tries_equal(node1->next, node2->next);
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

	bool empty_noaffect = true, add_delete = true;
	for (size_t i=0; i<n_rec; ++i) {
		records[i].key = gen_rand_str(gen_len_bw(1, N));
		for (size_t j=0; j<i; ++j)
			if (!strcmp(records[i].key, records[j].key)) {
				free(records[i--].key);
				continue;
			}
	}

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
				// fallthrough
			case BOTH:
				trie_insert(trie_b, key, malloc(10));
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
				// fallthrough
			case BOTH:
				trie_delete(trie_b, key);
				// fallthrough
			default:
				break;
			}
		}

		trie_delete(trie_a, "");
		trie_delete(trie_b, "");
		empty_noaffect = empty_noaffect
			&& trie_a->root && trie_a->root->segment[0] == '\0'
			&& !trie_a->root->next
			&& trie_b->root && trie_b->root->segment[0] == '\0'
			&& !trie_b->root->next;

		add_delete = add_delete
			&& tries_equal(trie_a->root, trie_b->root);
		trie_destroy(trie_a);
		trie_destroy(trie_b);
	}
	test_check(res, "Trie stays the same under corresponding add/deletes",
		   add_delete);
	test_check(res, "Empty value deletion does not affect structure",
		   empty_noaffect);

	for (size_t i=0; i<n_rec; ++i)
		free(records[i].key);
	free(records);
}


#if 0
TEST_DEFINE(test_, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

	/* Continue below */

	trie_destroy(trie);
}


TEST_DEFINE(test_, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

	/* Continue below */

	trie_destroy(trie);
}


TEST_DEFINE(test_, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

	/* Continue below */

	trie_destroy(trie);
}


TEST_DEFINE(test_, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

	/* Continue below */

	trie_destroy(trie);
}
#endif


TEST_START
(
	test_instantiation,
	vg_test_destroy,
	test_max_keylen,
	test_node_create,
	test_node_split,
	test_add_strs,
	test_node_merge,
	test_add_keybranch,
	test_insert,
	test_delete,
)
