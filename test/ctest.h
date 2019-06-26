#ifndef TEST
#define TEST


#include <stdlib.h>
#include <stdbool.h>
#include <sys/cdefs.h>


#define MAX_CHECKNAMES_PER_UNIT 256
#define N_RUNS_PER_TEST 1024
#define PRINT_WIDTH 64


struct test_result;
typedef struct test_result test_result_t;

typedef void (*test_t)(test_result_t* result);


void test_name(test_result_t* result, const char* name);
void test_acheck(test_result_t* result, bool check);
void test_check(test_result_t* result, const char* name, bool check);
int test_run(const test_t* tests, size_t n_tests, const char* module_name);


#define TEST_DEFINE(name, result) \
	__attribute_used__ void name(test_result_t* result)
#define TEST_AUTONAME(result) test_name(result, __func__)
#define TEST_START(...)							\
int main(void)								\
{									\
	void (*test_fns[])(test_result_t*) = {__VA_ARGS__};		\
	const size_t n_tests = sizeof test_fns / sizeof test_fns[0];	\
	return test_run(test_fns, n_tests, __FILE__);			\
}


#endif /* TEST */
