#ifndef TEST
#define TEST


#include <time.h>
#include <stdio.h>
#include <stdlib.h>


#define MAX_CHECKNAMES_PER_UNIT 256


typedef struct {
	char* fn_name;
	int passed, total;
	char* failed_checknames[MAX_CHECKNAMES_PER_UNIT];
	int n_names;
} __Tresult_t;


/** Note: __Tresult is reserved */
#define TEST_DEFINE(fn, code_block)					\
__Tresult_t fn(void)							\
{									\
	srand(time(NULL));						\
	__Tresult_t __Tresult = {.fn_name = __func__};			\
									\
	code_block;							\
									\
	return __Tresult;						\
}

#define TEST_ANONYMOUS_EXPECT(condition) {				\
	if (condition)							\
		++__Tresult.passed;					\
	++__Tresult.total;						\
}

#define TEST_NAMED_EXPECT(name, condition) {				\
	if (condition)							\
		++__Tresult.passed;					\
	++__Tresult.total;						\
	if (__Tresult.n_names < MAX_CHECKNAMES_PER_UNIT)		\
		__Tresult.failed_checknames[__Tresult.n_names++] = name;\
}

#define TEST_RUN(...) {							\
	int (*test_fns[])(void) = {__VA_ARGS__};			\
	size_t n_passed = 0;						\
	const size_t n_tests = sizeof test_fns / sizeof test_fns[0];	\
	fprintf(stderr, "--------------------------------\n");		\
	fprintf(stderr, "****** RUNNING TEST CASES ******\n");		\
	for (size_t i=0; i<n_tests; ++i) {				\
		__Tresult_t __Tresult = test_fns[i]();			\
		bool passed = __Tresult.passed == __Tresult.total;	\
		fprintf(stderr, "[%s] Test \"%s\": %d/%d checks"	\
			" passed", passed ? "PASS" : "FAIL",		\
			__Tresult.fn_name, __Tresult.passed,		\
			__Tresult.total);				\
		if (passed)						\
			++n_passed;					\
		if (__Tresult.n_names == 0) {				\
			fprintf(stderr, ".\n");				\
			continue;					\
		}							\
		fprintf(stderr, " (failed checks: %s",			\
			__Tresult.failed_checknames[0]);		\
		for (int j=1; j<__Tresult.n_names; ++j)			\
			fprintf(stderr, ", %s",				\
				__Tresult.failed_checknames[j]);	\
		fprintf(stderr, ").\n");				\
	}								\
	fprintf(stderr, "*********** ALL DONE ***********\n");		\
	bool passed = n_passed == n_tests;				\
	fprintf(stderr, "[%s] %d/%d test cases passed.\n",		\
		passed ? "PASS" : "FAIL", n_passed, n_tests);		\
	fprintf(stderr, "--------------------------------\n");		\
}


#undef MAX_CHECKNAMES_PER_UNIT


#endif /* TEST */
