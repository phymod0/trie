SRC_FILES := $(wildcard test_*.c)
INCLUDE_DIRS := ../src/ ../
TESTS := $(patsubst %.c,%,$(SRC_FILES))
CFLAGS := -g -Wall -Werror -Wextra
CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS))

all: $(TESTS)
$(TESTS):
	$(CC) $(CFLAGS) ctest.c $@.c $(patsubst test_%,../src/%.c,$@) -o $@
	./$@
	rm -rf $@
