/**
 * @file stack.h
 * @brief Methods for stack operations.
 */


#ifndef STACK
#define STACK


#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/** Operations on stack values. */
struct stack_ops {
	void (*dtor)(void*); /**< Destructor for an inserted value. */
};


static inline struct stack_ops stack_makeops(void (*dtor)(void*))
{
	struct stack_ops ops;
	ops.dtor = dtor;
	return ops;
}


/** Free all inserted values with <code>free()</code>. */
#define STACK_OPS_FREE stack_makeops(free)

/** Do not free inserted values. */
#define STACK_OPS_NONE stack_makeops(NULL)


/** Stack data structure. */
struct stack;
#ifndef STACK_FWD
#define STACK_FWD
typedef struct stack Stack;
#endif /* STACK_FWD */

Stack* stack_create(const struct stack_ops ops);
int stack_push(Stack* s, void* data);
void* stack_pop(Stack* s);
void* stack_top(Stack* s);
bool stack_empty(Stack* s);
void stack_destroy(Stack* s);


#endif /* STACK */
