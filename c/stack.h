#ifndef STACK_H
#define STACK_H

#define MAX_STACK 100

typedef struct
{
    char number[20];

} StackItem;

typedef struct
{
    StackItem data[MAX_STACK];
    int top;

} Stack;

void initializeStack(Stack *s);

int push(Stack *s, char number[]);

int pop(Stack *s,
        StackItem *item);

#endif