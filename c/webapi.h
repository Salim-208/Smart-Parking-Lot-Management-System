#ifndef WEBAPI_H
#define WEBAPI_H

#include <time.h>

#include "parking.h"   /* LOTS, SLOTS, VIP_SLOTS, parking[][] */

/* Plain, pointer-free structs so ctypes on the Python side can read them
   directly with no marshalling of BST/queue internals. */

typedef struct
{
    char number[20];
    char type[20];
    int  lot;          /* 0-indexed */
    int  slot;         /* 0-indexed */
    int  vip;
    time_t entryTime;           /* time_t, seconds since epoch */
    char entryTimeText[20];     /* "YYYY-MM-DD HH:MM:SS", formatted in C */
    long parkedSeconds;         /* now - entryTime, computed in C */

} WebVehicle;

typedef struct
{
    char number[20];
    char type[20];
    int  vip;

} WebQueueItem;

#ifdef _WIN32
  #define WEBAPI_EXPORT __declspec(dllexport)
#else
  #define WEBAPI_EXPORT
#endif

WEBAPI_EXPORT void wp_initialize(void);

/* returns: 1 = parked immediately, 0 = added to waiting queue, -1 = already parked/queued.
   On 1: outLot/outSlot are filled (0-indexed).
   On 0: outQueuePos is filled (1-indexed position in the queue). */
WEBAPI_EXPORT int wp_park(const char *number, const char *type, int vip,
                           int *outLot, int *outSlot, int *outQueuePos);

/* returns: 1 = exited, 0 = vehicle not found.
   On 1: outRecord, outFee and outHours are filled. Any waiting-queue vehicles
   that could now be seated are written into outAssigned (up to maxAssigned),
   and outAssignedCount is set to how many were seated. */
WEBAPI_EXPORT int wp_exit(const char *number,
                           WebVehicle *outRecord, double *outFee, int *outHours,
                           WebQueueItem *outAssigned, int maxAssigned, int *outAssignedCount);

/* returns: 0 = not found, 1 = parked (outVehicle filled), 2 = queued (outQueueItem filled) */
WEBAPI_EXPORT int wp_search(const char *number, WebVehicle *outVehicle, WebQueueItem *outQueueItem);

WEBAPI_EXPORT void wp_get_grid(int *outGrid /* LOTS*SLOTS ints, row-major */);

WEBAPI_EXPORT int wp_free_count(void);
WEBAPI_EXPORT int wp_used_count(void);
WEBAPI_EXPORT int wp_vip_free_count(void);
WEBAPI_EXPORT int wp_queue_count(void);

/* fills buffer with up to maxCount vehicles, in BST inorder (sorted by number).
   returns the number written. */
WEBAPI_EXPORT int wp_list_vehicles(WebVehicle *buffer, int maxCount);

/* fills buffer with up to maxCount waiting-queue entries, front to back.
   returns the number written. */
WEBAPI_EXPORT int wp_list_queue(WebQueueItem *buffer, int maxCount);

WEBAPI_EXPORT double wp_get_rate(const char *type);

/* calls the original saveRecords() (from file.c), which writes
   parking_records.txt in the process's working directory. */
WEBAPI_EXPORT void wp_save_records(void);

#endif
