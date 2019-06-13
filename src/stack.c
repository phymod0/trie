#include "stack.h"


#define ALLOC(x) (x = malloc(sizeof *(x)))


typedef struct stack_elem {
	void* val;
	struct stack_elem* next;
} stack_elem_t;

typedef struct {
	stack_elem_t* head;
	struct stack_ops* ops;
} stack_priv_t;


Stack* stack_create(struct stack_ops* ops)
{
	Stack* s;
	stack_priv_t* priv = NULL;
	if (!ALLOC(s) || !ALLOC(priv) || !ALLOC(priv->ops)) {
		free(s);
		free(priv);
		return NULL;
	}
	priv->head = NULL;
	memcpy(priv->ops, ops, sizeof *(priv->ops));
	s->priv = priv;
	return s;
}


int stack_push(Stack* s, void* data)
{
	stack_priv_t* priv = s->priv;

	stack_elem_t* head_new;
	if (!ALLOC(head_new))
		return -1;
	head_new->val = data;
	head_new->next = priv->head;

	priv->head = head_new;
}


void* stack_pop(Stack* s)
{
	stack_priv_t* priv = s->priv;
	stack_elem_t* head = priv->head;

	if (!head)
		return NULL;
	void* val = head->val;
	priv->head = head->next;
	free(head);
	return val;
}


bool stack_empty(Stack* s)
{
	stack_priv_t* priv = s->priv;

	return priv->head == NULL;
}


void stack_destroy(Stack* s)
{
	stack_priv_t* priv = s->priv;
	stack_elem_t* elem = priv->head;
	void (*dtor)(void*) = priv->ops->dtor;

	while (elem) {
		stack_elem_t* next = elem->next;
		if (dtor)
			dtor(elem->val);
		free(elem);
		elem = next;
	}
	free(priv);
	free(s);
}


#undef ALLOC
