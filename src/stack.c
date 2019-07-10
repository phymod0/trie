#include "stack.h"


#define ALLOC(x, type) (x = (type*)malloc(sizeof *(x)))


typedef struct stack_elem {
	void* val;
	struct stack_elem* next;
} StackElem;

struct stack {
	StackElem* head;
	struct stack_ops ops;
};
#ifndef STACK_FWD
#define STACK_FWD
typedef struct stack Stack;
#endif /* STACK_FWD */


Stack* stack_create(const struct stack_ops ops)
{
	Stack* s;
	if (!ALLOC(s, Stack)) {
		free(s);
		return NULL;
	}
	s->head = NULL;
	s->ops = ops;
	return s;
}


int stack_push(Stack* s, void* data)
{
	StackElem* head_new;
	if (!ALLOC(head_new, StackElem))
		return -1;
	head_new->val = data;
	head_new->next = s->head;

	s->head = head_new;
	return 0;
}


void* stack_pop(Stack* s)
{
	StackElem* head = s->head;

	if (!head)
		return NULL;
	void* val = head->val;
	s->head = head->next;
	free(head);
	return val;
}


void* stack_top(Stack* s)
{
	return s->head ? s->head->val : NULL;
}


bool stack_empty(Stack* s)
{
	return s->head == NULL;
}


void stack_destroy(Stack* s)
{
	if (!s)
		return;

	StackElem* elem = s->head;
	void (*dtor)(void*) = s->ops.dtor;

	while (elem) {
		StackElem* next = elem->next;
		if (dtor)
			dtor(elem->val);
		free(elem);
		elem = next;
	}
	free(s);
}


#undef ALLOC
