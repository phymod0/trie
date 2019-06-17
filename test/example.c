#include "ctest.h"


TEST_DEFINE(test_add_ints, res)
{
	TEST_AUTONAME(res);
	int a = 2, b = 3;
	test_acheck(res, a+b == 5);
	test_check(res, "Multiplication", a*b == 6);
}

TEST_DEFINE(test_hello_reverse, res)
{
	// TEST_AUTONAME(res);
	char arr[] = "hello";
	char rev[] = "olleh";
	bool equal = true;
	size_t len = (sizeof arr) - 1;
	for (size_t i=0; i<len; ++i)
		if (arr[i] != rev[len-i-1])
			equal = false;
	test_check(res, "Reverse test", equal);
}


int main(void)
{
	TEST_RUN(
		test_add_ints,
		test_hello_reverse,
	);

	return 0;
}
