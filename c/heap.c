#include "heap.h"

void initializeHeap(MinHeap *heap)
{
    heap->size = 0;
}

int smaller(Slot a, Slot b)
{
    if(a.lot != b.lot)
        return a.lot < b.lot;

    return a.slot < b.slot;
}

void heapifyUp(MinHeap *heap, int index)
{
    while(index > 0)
    {
        int parent = (index - 1) / 2;

        if(smaller(heap->data[parent],
                   heap->data[index]))
            break;

        Slot temp = heap->data[parent];

        heap->data[parent] =
            heap->data[index];

        heap->data[index] = temp;

        index = parent;
    }
}

void heapifyDown(MinHeap *heap, int index)
{
    while(1)
    {
        int left = 2 * index + 1;
        int right = 2 * index + 2;

        int smallest = index;

        if(left < heap->size &&
           smaller(heap->data[left],
                   heap->data[smallest]))
        {
            smallest = left;
        }

        if(right < heap->size &&
           smaller(heap->data[right],
                   heap->data[smallest]))
        {
            smallest = right;
        }

        if(smallest == index)
            break;

        Slot temp = heap->data[index];

        heap->data[index] =
            heap->data[smallest];

        heap->data[smallest] = temp;

        index = smallest;
    }
}

void buildHeap(MinHeap *heap, int vip)
{
    heap->size = 0;

    int start;

    if(vip)
        start = 0;
    else
        start = VIP_SLOTS;

    for(int i = 0; i < LOTS; i++)
    {
        for(int j = start; j < SLOTS; j++)
        {
            if(parking[i][j] == 0)
            {
                heap->data[heap->size].lot = i;
                heap->data[heap->size].slot = j;

                heapifyUp(heap,
                          heap->size);

                heap->size++;
            }
        }
    }
}

int removeMinimum(MinHeap *heap,
                  Slot *slot)
{
    if(heap->size == 0)
        return 0;

    *slot = heap->data[0];

    heap->size--;

    if(heap->size > 0)
    {
        heap->data[0] =
            heap->data[heap->size];

        heapifyDown(heap, 0);
    }

    return 1;
}