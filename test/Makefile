INCLUDE_DIRS := ../src/ ../
CFLAGS := -g -Wall -Werror -Wextra
CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS))

SRC_FILES := $(wildcard test_*.c)
TESTS := $(patsubst %.c,%,$(SRC_FILES))

all: $(TESTS)
$(TESTS):
	$(CC) $(CFLAGS) ctest.c $@.c -o $@
	./$@
	rm -rf $@
