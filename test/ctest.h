#ifndef TEST
#define TEST


#include <stdlib.h>
#include <stdbool.h>


#define MAX_CHECKNAMES_PER_UNIT 256


struct test_result;
typedef struct test_result test_result_t;

typedef void (*test_t)(test_result_t* result);


void test_name(test_result_t* result, const char* name);
void test_acheck(test_result_t* result, bool check);
void test_check(test_result_t* result, const char* name, bool check);
int test_run(const test_t* tests, size_t n_tests);


#define TEST_DEFINE(name, result) void name(test_result_t* result)
#define TEST_AUTONAME(result) test_name(result, __func__)
#define TEST_START(...)							\
int main(void)								\
{									\
	void (*test_fns[])(test_result_t*) = {__VA_ARGS__};		\
	const size_t n_tests = sizeof test_fns / sizeof test_fns[0];	\
	return test_run(test_fns, n_tests);				\
}


#endif /* TEST */
