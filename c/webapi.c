#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "webapi.h"
#include "bst.h"
#include "queue.h"
#include "heap.h"
#include "hash.h"
#include "stack.h"
#include "fee.h"
#include "file.h"

/* Same state main.c used to keep on the stack, now kept for the life of the
   process so each HTTP request can pick up where the last one left off. */
static Vehicle    *g_root;
static Queue        g_queue;
static MinHeap       g_heap;
static HashTable     g_hash;
static Stack         g_stack;

void wp_initialize(void)
{
    g_root = NULL;

    initializeParking();
    initializeQueue(&g_queue);
    initializeHeap(&g_heap);
    initializeHash(&g_hash);
    initializeStack(&g_stack);
}

/* Fills the display-only fields (formatted timestamp, elapsed seconds) that
   depend on "now" -- kept in C so the Python side never has to touch time
   arithmetic itself, only decode/serve what this library already computed. */
static void fillDerived(WebVehicle *v)
{
    time_t now = time(NULL);

    struct tm *tmInfo = localtime(&v->entryTime);
    strftime(v->entryTimeText, sizeof(v->entryTimeText), "%Y-%m-%d %H:%M:%S", tmInfo);

    double seconds = difftime(now, v->entryTime);
    v->parkedSeconds = (seconds > 0) ? (long)seconds : 0;
}

static int queueHasNumber(const char *number)
{
    int index = g_queue.front;

    for(int i = 0; i < g_queue.count; i++)
    {
        if(strcmp(g_queue.data[index].number, number) == 0)
            return 1;

        index = (index + 1) % MAX_QUEUE;
    }

    return 0;
}

int wp_park(const char *number, const char *type, int vip,
            int *outLot, int *outSlot, int *outQueuePos)
{
    if(searchVehicle(g_root, (char *)number) != NULL)
        return -1;

    if(queueHasNumber(number))
        return -1;

    buildHeap(&g_heap, vip);

    Slot slot;

    if(removeMinimum(&g_heap, &slot))
    {
        Vehicle *newVehicle = malloc(sizeof(Vehicle));

        strcpy(newVehicle->number, number);
        strcpy(newVehicle->type, type);

        newVehicle->lot = slot.lot;
        newVehicle->slot = slot.slot;
        newVehicle->vip = vip;
        newVehicle->entryTime = time(NULL);

        newVehicle->left = NULL;
        newVehicle->right = NULL;

        parking[slot.lot][slot.slot] = 1;

        g_root = insertVehicle(g_root, newVehicle);

        insertHash(&g_hash, (char *)number, slot.lot, slot.slot);

        push(&g_stack, (char *)number);

        *outLot = slot.lot;
        *outSlot = slot.slot;

        return 1;
    }
    else
    {
        enqueue(&g_queue, (char *)number, (char *)type, vip);

        *outQueuePos = g_queue.count;

        return 0;
    }
}

/* Tries to seat the vehicle currently at the front of the waiting queue,
   repeatedly, stopping as soon as the front vehicle can't be placed - the
   queue is strict FIFO, mirroring how enqueue()/dequeue() already behave. */
static int fillFromQueue(WebQueueItem *outAssigned, int maxAssigned)
{
    int assignedCount = 0;

    while(g_queue.count > 0)
    {
        QueueVehicle peek = g_queue.data[g_queue.front];

        buildHeap(&g_heap, peek.vip);

        Slot slot;

        if(!removeMinimum(&g_heap, &slot))
            break;

        QueueVehicle popped;
        dequeue(&g_queue, &popped);

        Vehicle *newVehicle = malloc(sizeof(Vehicle));

        strcpy(newVehicle->number, popped.number);
        strcpy(newVehicle->type, popped.type);

        newVehicle->lot = slot.lot;
        newVehicle->slot = slot.slot;
        newVehicle->vip = popped.vip;
        newVehicle->entryTime = time(NULL);

        newVehicle->left = NULL;
        newVehicle->right = NULL;

        parking[slot.lot][slot.slot] = 1;

        g_root = insertVehicle(g_root, newVehicle);

        insertHash(&g_hash, popped.number, slot.lot, slot.slot);

        push(&g_stack, popped.number);

        if(outAssigned != NULL && assignedCount < maxAssigned)
        {
            strcpy(outAssigned[assignedCount].number, popped.number);
            strcpy(outAssigned[assignedCount].type, popped.type);
            outAssigned[assignedCount].vip = popped.vip;
        }

        assignedCount++;
    }

    return assignedCount;
}

int wp_exit(const char *number,
            WebVehicle *outRecord, double *outFee, int *outHours,
            WebQueueItem *outAssigned, int maxAssigned, int *outAssignedCount)
{
    Vehicle *vehicle = searchVehicle(g_root, (char *)number);

    if(vehicle == NULL)
        return 0;

    double seconds = difftime(time(NULL), vehicle->entryTime);
    double hours = seconds / 3600.0;

    if(hours < 1)
        hours = 1;

    int chargedHours = (int)hours;

    if(hours > chargedHours)
        chargedHours++;

    double fee = chargedHours * getRate(vehicle->type);
    fee = round(fee * 100.0) / 100.0;   /* round to 2 decimal places, in C */

    strcpy(outRecord->number, vehicle->number);
    strcpy(outRecord->type, vehicle->type);
    outRecord->lot = vehicle->lot;
    outRecord->slot = vehicle->slot;
    outRecord->vip = vehicle->vip;
    outRecord->entryTime = vehicle->entryTime;

    *outFee = fee;
    *outHours = chargedHours;

    parking[vehicle->lot][vehicle->slot] = 0;

    deleteHash(&g_hash, (char *)number);

    g_root = deleteVehicle(g_root, (char *)number);

    *outAssignedCount = fillFromQueue(outAssigned, maxAssigned);

    return 1;
}

int wp_search(const char *number, WebVehicle *outVehicle, WebQueueItem *outQueueItem)
{
    Vehicle *vehicle = searchVehicle(g_root, (char *)number);

    if(vehicle != NULL)
    {
        strcpy(outVehicle->number, vehicle->number);
        strcpy(outVehicle->type, vehicle->type);
        outVehicle->lot = vehicle->lot;
        outVehicle->slot = vehicle->slot;
        outVehicle->vip = vehicle->vip;
        outVehicle->entryTime = vehicle->entryTime;

        fillDerived(outVehicle);

        return 1;
    }

    int index = g_queue.front;

    for(int i = 0; i < g_queue.count; i++)
    {
        if(strcmp(g_queue.data[index].number, number) == 0)
        {
            strcpy(outQueueItem->number, g_queue.data[index].number);
            strcpy(outQueueItem->type, g_queue.data[index].type);
            outQueueItem->vip = g_queue.data[index].vip;

            return 2;
        }

        index = (index + 1) % MAX_QUEUE;
    }

    return 0;
}

void wp_get_grid(int *outGrid)
{
    for(int i = 0; i < LOTS; i++)
        for(int j = 0; j < SLOTS; j++)
            outGrid[i * SLOTS + j] = parking[i][j];
}

int wp_free_count(void)
{
    int free_ = 0;

    for(int i = 0; i < LOTS; i++)
        for(int j = 0; j < SLOTS; j++)
            if(parking[i][j] == 0)
                free_++;

    return free_;
}

int wp_used_count(void)
{
    return (LOTS * SLOTS) - wp_free_count();
}

int wp_vip_free_count(void)
{
    int free_ = 0;

    for(int i = 0; i < LOTS; i++)
        for(int j = 0; j < VIP_SLOTS; j++)
            if(parking[i][j] == 0)
                free_++;

    return free_;
}

int wp_queue_count(void)
{
    return g_queue.count;
}

static int inorderFill(Vehicle *node, WebVehicle *buffer, int maxCount, int written)
{
    if(node == NULL || written >= maxCount)
        return written;

    written = inorderFill(node->left, buffer, maxCount, written);

    if(written < maxCount)
    {
        strcpy(buffer[written].number, node->number);
        strcpy(buffer[written].type, node->type);
        buffer[written].lot = node->lot;
        buffer[written].slot = node->slot;
        buffer[written].vip = node->vip;
        buffer[written].entryTime = node->entryTime;
        fillDerived(&buffer[written]);
        written++;
    }

    written = inorderFill(node->right, buffer, maxCount, written);

    return written;
}

int wp_list_vehicles(WebVehicle *buffer, int maxCount)
{
    return inorderFill(g_root, buffer, maxCount, 0);
}

int wp_list_queue(WebQueueItem *buffer, int maxCount)
{
    int index = g_queue.front;
    int written = 0;

    for(int i = 0; i < g_queue.count && written < maxCount; i++)
    {
        strcpy(buffer[written].number, g_queue.data[index].number);
        strcpy(buffer[written].type, g_queue.data[index].type);
        buffer[written].vip = g_queue.data[index].vip;

        written++;
        index = (index + 1) % MAX_QUEUE;
    }

    return written;
}

double wp_get_rate(const char *type)
{
    return getRate((char *)type);
}

void wp_save_records(void)
{
    saveRecords(g_root);
}
