#ifndef QUEUE_H
#define QUEUE_H

#define MAX_QUEUE 100

typedef struct
{
    char number[20];
    char type[20];
    int vip;

} QueueVehicle;

typedef struct
{
    QueueVehicle data[MAX_QUEUE];

    int front;
    int rear;
    int count;

} Queue;

void initializeQueue(Queue *q);

int enqueue(Queue *q,
            char number[],
            char type[],
            int vip);

int dequeue(Queue *q,
            QueueVehicle *vehicle);

void displayQueue(Queue *q);

#endif