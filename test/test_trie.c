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


TEST_DEFINE(test_, res)
{
	TEST_AUTONAME(res);

	/* Continue below */
}


TEST_START
(
	test_instantiation,
	test_,
)
