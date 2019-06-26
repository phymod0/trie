#include "stack.c"

#include "ctest.h"


TEST_DEFINE(test_stack_create, res)
{
	TEST_AUTONAME(res);

	Stack* s = stack_create(STACK_OPS_FREE);

	test_check(res, "Stack initially empty", stack_empty(s));
	test_check(res, "Pop and top initially return NULL", !stack_pop(s)
		   && !stack_pop(s) && !stack_pop(s) && !stack_top(s)
		   && !stack_top(s));

	stack_destroy(s);
}


TEST_DEFINE(test_stack_pushpop, res)
{
	TEST_AUTONAME(res);

	Stack* s = stack_create(STACK_OPS_FREE);
	const char* message = "racecar Hello world! wasitacatisaw";
	const char* reverse = "wasitacatisaw !dlrow olleH racecar";

	bool match = true;
	for (; *message; ++message) {
		char* dup = malloc(1);
		*dup = *message;
		stack_push(s, dup);
	}
	for (; *reverse; ++reverse) {
		char* c = stack_pop(s);
		if (*c != *reverse) {
			match = false;
			free(c);
			break;
		}
		free(c);
	}
	test_check(res, "Stack popped in reverse push order", match);
	test_check(res, "Stack empty after full removal", stack_empty(s));

	stack_destroy(s);
}


TEST_DEFINE(test_stack_top, res)
{
	TEST_AUTONAME(res);

	Stack* s = stack_create(STACK_OPS_FREE);
	const char* message = "Hello world!";

	bool match = true;
	for (; *message; ++message) {
		char* dup = malloc(1);
		*dup = *message;
		stack_push(s, dup);
		if (!stack_top(s) || *(char*)stack_top(s) != *message)
			match = false;
	}
	test_check(res, "Top correct while pushing", match);

	match = true;
	message = "!dlrow olleH";
	for (; *message; ++message) {
		char* c = stack_top(s);
		c = stack_top(s);
		c = stack_top(s);
		c = stack_top(s);
		if (!c || *c != *message) {
			match = false;
			free(c);
			break;
		}
		free(c);
		stack_pop(s);
	}
	test_check(res, "Top correct while popping", match);

	stack_destroy(s);
}


TEST_DEFINE(vg_test_stack_destroy, res)
{
	TEST_AUTONAME(res);

	Stack* s = stack_create(STACK_OPS_FREE);

	size_t n_push = (rand() % 1000) + 1000;
	while (n_push --> 0) {
		int* r = malloc(sizeof(int));
		stack_push(s, r);
	}

	stack_destroy(s);
}


TEST_START
(
	test_stack_create,
	test_stack_pushpop,
	test_stack_top,
	vg_test_stack_destroy,
)
