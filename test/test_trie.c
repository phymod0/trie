#include "trie.h"
#include "trie.c"
#include "stack.c"

#include "ctest.h"


TEST_DEFINE(test_instantiation, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);

	bool trie_null = !trie;
	if (trie_null) {
		test_check(res, "NULL on out-of-memory", trie_null);
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


TEST_DEFINE(vg_test_destroy, res)
{
	TEST_AUTONAME(res);

	Trie* trie = trie_create(TRIE_OPS_FREE);
	Trie* trie2 = trie_create(TRIE_OPS_NONE);

	size_t t = gen_len_bw(100, 200);
	while (t --> 0) {
		char* seg = gen_rand_str(t);
		void* junk = malloc(10);
		trie_insert(trie, seg, junk);
		trie_insert(trie2, seg, junk);
		free(seg);
	}

	trie_destroy(trie);
	trie_destroy(trie2);
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


TEST_DEFINE(test_node_create, res)
{
	TEST_AUTONAME(res);

	char* seg = gen_rand_str(gen_len_bw(100, 200));
	void* value = malloc(100);
	trie_node_t* node = node_create(seg, value);
	if (!node) {
		test_check(res, "Node is NULL on failure", true);
		return;
	}
	test_check(res, "Pointers are initially NULL", !node->fchild &&
		   !node->next);
	bool seg_match = node->segment && strcmp(node->segment, seg) == 0;
	bool val_match = node->value && memcmp(node->value, value, 100) == 0;
	test_check(res, "Segment and value match", seg_match && val_match);

	free(node->segment);
	free(node->value);
	free(node);
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
)
