INCLUDE_DIRS := ../src/ ../
CFLAGS := -g -Wall -Werror -Wextra -std=gnu11 -fsanitize=address \
	-fsanitize=undefined -Winline # -O3 -finline-functions -Winline
CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS))
OPT_CFLAGS := -Wall -Werror -Wextra -O3 -Winline -finline-functions
OPT_CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS))

SRC_FILES := $(wildcard test_*.c)
TESTS := $(patsubst %.c,%,$(SRC_FILES))
EXEC := $(patsubst %.c,.__test_exec_%,$(SRC_FILES))

compile: $(TESTS)
check: $(EXEC) clean
clean:
	@rm -rf $(TESTS)
.DELETE_ON_ERROR:
$(EXEC):
	@$(CC) $(CFLAGS) ctest.c $(patsubst .__test_exec_%,%.c,$@) -o $@
	@./$@
	@rm -rf $@
$(TESTS):
	@$(CC) $(OPT_CFLAGS) ctest.c $@.c -o $@
