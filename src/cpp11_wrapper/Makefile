all:
	cp ../trie.* ../stack.* .
	g++ -o test trie.cpp trie.c stack.c -ansi -pedantic -std=c++11 -Wall -Werror -Wextra -fsanitize=address -fsanitize=undefined
	./test
	rm -rf test
	rm trie.h trie.c stack.h stack.c
