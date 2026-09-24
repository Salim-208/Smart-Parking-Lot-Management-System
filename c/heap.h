#ifndef HEAP_H
#define HEAP_H

#include "parking.h"

typedef struct
{
    int lot;
    int slot;

} Slot;

typedef struct
{
    Slot data[LOTS * SLOTS];
    int size;

} MinHeap;

void initializeHeap(MinHeap *heap);

void buildHeap(MinHeap *heap,
               int vip);

int removeMinimum(MinHeap *heap,
                  Slot *slot);

#endif