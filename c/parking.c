#include <stdio.h>
#include "parking.h"

int parking[LOTS][SLOTS];

void initializeParking()
{
    for(int i = 0; i < LOTS; i++)
    {
        for(int j = 0; j < SLOTS; j++)
        {
            parking[i][j] = 0;
        }
    }
}

void showParking()
{
    printf("\n========== PARKING STATUS ==========\n");

    for(int i = 0; i < LOTS; i++)
    {
        printf("Lot %d: ", i + 1);

        for(int j = 0; j < SLOTS; j++)
        {
            if(parking[i][j] == 0)
                printf("[FREE] ");
            else
                printf("[USED] ");
        }

        printf("\n");
    }
}