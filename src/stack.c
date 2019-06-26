#include "stack.h"


#define ALLOC(x) (x = malloc(sizeof *(x)))


typedef struct stack_elem {
	void* val;
	struct stack_elem* next;
} stack_elem_t;

typedef struct stack {
	stack_elem_t* head;
	struct stack_ops ops;
} Stack;


Stack* stack_create(const struct stack_ops* ops)
{
	Stack* s;
	if (!ALLOC(s)) {
		free(s);
		return NULL;
	}
	s->head = NULL;
	s->ops = *ops;
	return s;
}


int stack_push(Stack* s, void* data)
{
	stack_elem_t* head_new;
	if (!ALLOC(head_new))
		return -1;
	head_new->val = data;
	head_new->next = s->head;

	s->head = head_new;
	return 0;
}


void* stack_pop(Stack* s)
{
	stack_elem_t* head = s->head;

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

	stack_elem_t* elem = s->head;
	void (*dtor)(void*) = s->ops.dtor;

	while (elem) {
		stack_elem_t* next = elem->next;
		if (dtor)
			dtor(elem->val);
		free(elem);
		elem = next;
	}
	free(s);
}


#undef ALLOC
