#include "ctest.h"

#include <time.h>
#include <stdio.h>
#include <string.h>


#define PSTDOUT(...) fprintf(stdout, __VA_ARGS__)
#define PSTDERR(...) fprintf(stderr, __VA_ARGS__)

#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define YELLOW "\x1B[33m"
#define RESET "\033[01;30m"


struct test_result {
	const char* test_name;
	unsigned long long passed, total;
	const char* failed_checknames[MAX_CHECKNAMES_PER_UNIT];
	unsigned long long n_failed_names;
};

#ifndef TR_FWD
#define TR_FWD
typedef struct test_result TestResult;
#endif /* TR_FWD */


static const char* status_str(bool condition)
{
	return condition ? GREEN "PASS" RESET : RED "FAIL" RESET;
}


static void print_test_results(TestResult* result)
{
	const char* status = status_str(result->passed == result->total);
	const char* test_name = result->test_name;
	if (!test_name)
		test_name = "(unnamed)";
	PSTDOUT("[%s] Test %s: %llu/%llu checks passed\n", status, test_name,
		result->passed, result->total);
	if (result->n_failed_names == 0)
		return;
	for (unsigned long long i=0; i<result->n_failed_names; ++i) {
		PSTDOUT("\t[" RED "Failed #%llu" RESET "] ", i + 1);
		PSTDOUT("%s\n", result->failed_checknames[i]);
	}
}


static bool run_single_test(test_t test)
{
	srand(time(NULL));

	TestResult result;
	for (int i=0; i<N_RUNS_PER_TEST; ++i) {
		result = (TestResult){0};
		test(&result);
		if (result.passed != result.total)
			break;
	}
	print_test_results(&result);

	return result.passed == result.total;
}


static void print_bar(const char* color, const char* hdr)
{
	size_t hdr_len = strlen(hdr), n_dashes = PRINT_WIDTH - 4 - hdr_len;
	for (size_t i=0; i<(n_dashes+1)/2; ++i)
		PSTDOUT("-");
	PSTDOUT("[ %s%s%s ]", color, hdr, RESET);
	for (size_t i=0; i<n_dashes/2; ++i)
		PSTDOUT("-");
	PSTDOUT("\n");
}


void test_acheck(TestResult* result, bool check)
{
	++result->total;
	if (check)
		++result->passed;
}


void test_check(TestResult* result, const char* name, bool check)
{
	++result->total;
	if (check) {
		++result->passed;
		return;
	}
	if (result->n_failed_names < MAX_CHECKNAMES_PER_UNIT)
		result->failed_checknames[result->n_failed_names++] = name;
	else
		PSTDERR("Cannot record check failure name (buffer full).\n");
}


void test_name(TestResult* result, const char* name)
{
	result->test_name = name;
}


int test_run(const test_t* tests, size_t n_tests, const char* module_name)
{
	size_t n_passed = 0;
	PSTDOUT("\n");
	print_bar(RESET, module_name);
	for (size_t i=0; i<n_tests; ++i)
		if (run_single_test(tests[i]))
			++n_passed;
	bool passed = n_passed == n_tests;
	char result[PRINT_WIDTH - 4];
	snprintf(result, sizeof result, "Passed %lu/%lu", n_passed, n_tests);
	print_bar(passed ? GREEN : RED, result);
	PSTDOUT("\n");
	return (int)(n_tests - n_passed);
}


#undef RESET
#undef YELLOW
#undef BLUE
#undef GREEN
#undef RED

#undef PSTDERR
#undef PSTDOUT


#undef ERROR
