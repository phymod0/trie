#include "test.h"

#if 0
#include "../src/trie.h"
#include "../src/stack.h"
#endif


TEST_DEFINE(test_int_ops, {
	int a, b;
	a = 2, b = 3;

	TEST_ANONYMOUS_EXPECT(a+b == 5);
	TEST_NAMED_EXPECT("Multiplication", a*b == 7);
})

TEST_DEFINE(test_string_reverse, {
	char arr[] = "Hello world!";
	char reversed[sizeof arr] = {0};
	for (int i=0; i<(sizeof arr)/2; ++i)
		reversed[i] = arr[(sizeof arr) - i - 1];
	for (int i=0, j=sizeof(arr)-1; i<sizeof arr; ++i, --j)
		TEST_ANONYMOUS_EXPECT(arr[i] == reversed[j]);
})


int main(void)
{
	TEST_RUN(test_int_ops);
	TEST_RUN(test_string_reverse);

	return 0;
}
