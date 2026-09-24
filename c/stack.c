#include <string.h>

#include "stack.h"

void initializeStack(Stack *s)
{
    s->top = -1;
}

int push(Stack *s, char number[])
{
    if(s->top == MAX_STACK - 1)
        return 0;

    s->top++;

    strcpy(s->data[s->top].number,
           number);

    return 1;
}

int pop(Stack *s,
        StackItem *item)
{
    if(s->top == -1)
        return 0;

    *item = s->data[s->top];

    s->top--;

    return 1;
}