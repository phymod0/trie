INCLUDE_DIRS := ../src/ ../
CFLAGS := -g -Wall -Werror -Wextra
CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS))

SRC_FILES := $(wildcard test_*.c)
TESTS := $(patsubst %.c,%,$(SRC_FILES))

.DELETE_ON_ERROR:
check: $(TESTS)
	@rm -rf $(TESTS)
$(TESTS):
	@$(CC) $(CFLAGS) ctest.c $@.c -o $@
	@./$@
	@rm -rf $(TESTS)
