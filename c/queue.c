#include <stdio.h>
#include <string.h>

#include "queue.h"

void initializeQueue(Queue *q)
{
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

int enqueue(Queue *q,
            char number[],
            char type[],
            int vip)
{
    if(q->count == MAX_QUEUE)
        return 0;

    q->rear =
        (q->rear + 1) % MAX_QUEUE;

    strcpy(q->data[q->rear].number, number);
    strcpy(q->data[q->rear].type, type);

    q->data[q->rear].vip = vip;

    q->count++;

    return 1;
}

int dequeue(Queue *q,
            QueueVehicle *vehicle)
{
    if(q->count == 0)
        return 0;

    *vehicle = q->data[q->front];

    q->front =
        (q->front + 1) % MAX_QUEUE;

    q->count--;

    return 1;
}

void displayQueue(Queue *q)
{
    if(q->count == 0)
    {
        printf("Waiting queue is empty.\n");
        return;
    }

    int index = q->front;

    printf("\n========== WAITING QUEUE ==========\n");

    for(int i = 0; i < q->count; i++)
    {
        printf("%d. %s | %s | %s\n",
               i + 1,
               q->data[index].number,
               q->data[index].type,
               q->data[index].vip ?
               "VIP" : "Normal");

        index =
            (index + 1) % MAX_QUEUE;
    }
}