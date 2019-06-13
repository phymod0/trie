#ifndef STACK
#define STACK


#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define STACK_OPS_FREE		\
	&(struct stack_ops){	\
		.dtor = free,	\
	}
#define STACK_OPS_NULL		\
	&(struct stack_ops){	\
		.dtor = NULL,	\
	}


typedef struct {void* priv;} Stack;
struct stack_ops {
	void (*dtor)(void*);
};


Stack* stack_create(struct stack_ops* ops);
int stack_push(Stack* s, void* data);
void* stack_pop(Stack* s);
bool stack_empty(Stack* s);
void stack_destroy(Stack* s);


#endif /* STACK */
