INCLUDE_DIRS := ../src/ ../

CFLAGS_COMMON := -Wall -Werror -Wextra -Winline -std=c99 -pedantic
CFLAGS_COMMON += $(patsubst %,-I%,$(INCLUDE_DIRS))

TEST_CFLAGS := -g -fsanitize=address -fsanitize=undefined
TEST_CFLAGS += $(CFLAGS_COMMON)

OPT_CFLAGS := -O3 -finline-functions
OPT_CFLAGS += $(CFLAGS_COMMON)

SRC_FILES := $(wildcard test_*.c)
TESTS := $(patsubst %.c,%,$(SRC_FILES))
EXEC := $(patsubst %.c,.__test_exec_%,$(SRC_FILES))

compile: $(TESTS)
check: $(EXEC) clean
clean:
	@rm -rf $(TESTS)
.DELETE_ON_ERROR:
$(EXEC):
	@$(CC) $(TEST_CFLAGS) ctest.c $(patsubst .__test_exec_%,%.c,$@) -o $@
	@./$@
	@rm -rf $@
$(TESTS):
	@$(CC) $(OPT_CFLAGS) ctest.c $@.c -o $@
