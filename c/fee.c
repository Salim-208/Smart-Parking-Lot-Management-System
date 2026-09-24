#include <stdio.h>
#include <time.h>
#include<string.h>

#include "fee.h"

double getRate(char type[])
{
    if(strcmp(type, "bike") == 0)
        return 20.0;

    if(strcmp(type, "truck") == 0)
        return 80.0;

    return 50.0;
}

double calculateFee(Vehicle *vehicle)
{
    time_t currentTime = time(NULL);

    double seconds =
        difftime(currentTime,
                 vehicle->entryTime);

    double hours = seconds / 3600.0;

    if(hours < 1)
        hours = 1;

    int chargedHours = (int)hours;

    if(hours > chargedHours)
        chargedHours++;

    return chargedHours *
           getRate(vehicle->type);
}

void printBill(Vehicle *vehicle)
{
    double fee = calculateFee(vehicle);

    printf("\n========== PARKING BILL ==========\n");

    printf("Vehicle : %s\n",
           vehicle->number);

    printf("Type    : %s\n",
           vehicle->type);

    printf("Lot     : %d\n",
           vehicle->lot + 1);

    printf("Slot    : %d\n",
           vehicle->slot + 1);

    printf("Fee     : %.2f\n",
           fee);
}