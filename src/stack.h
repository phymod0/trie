/**
 * @file stack.h
 * @brief Methods for stack operations.
 */


#ifndef STACK
#define STACK


#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/** Free all inserted values with <code>free()</code>. */
#define STACK_OPS_FREE		\
	&(struct stack_ops){	\
		.dtor = free,	\
	}

/** Do not free inserted values. */
#define STACK_OPS_NONE		\
	&(struct stack_ops){	\
		.dtor = NULL,	\
	}


/** Stack data structure. */
struct stack;
typedef struct stack Stack;

/** Operations on stack values. */
struct stack_ops {
	void (*dtor)(void*); /**< Destructor for an inserted value. */
};


Stack* stack_create(struct stack_ops* ops);
int stack_push(Stack* s, void* data);
void* stack_pop(Stack* s);
void* stack_top(Stack* s);
bool stack_empty(Stack* s);
void stack_destroy(Stack* s);


#endif /* STACK */
