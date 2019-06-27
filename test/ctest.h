#ifndef TEST
#define TEST


#include <stdlib.h>
#include <stdbool.h>
#include <sys/cdefs.h>


#define MAX_CHECKNAMES_PER_UNIT 256
#define N_RUNS_PER_TEST 1024
#define PRINT_WIDTH 64


struct test_result;

#ifndef TR_FWD
#define TR_FWD
typedef struct test_result TestResult;
#endif /* TR_FWD */

typedef void (*test_t)(TestResult* result);


void test_name(TestResult* result, const char* name);
void test_acheck(TestResult* result, bool check);
void test_check(TestResult* result, const char* name, bool check);
int test_run(const test_t* tests, size_t n_tests, const char* module_name);


#define TEST_DEFINE(name, result) \
	__attribute_used__ void name(TestResult* result)
#define TEST_AUTONAME(result) test_name(result, __func__)
#define TEST_START(...)							\
int main(void)								\
{									\
	void (*test_fns[])(TestResult*) = {__VA_ARGS__};		\
	const size_t n_tests = sizeof test_fns / sizeof test_fns[0];	\
	return test_run(test_fns, n_tests, __FILE__);			\
}


#endif /* TEST */
