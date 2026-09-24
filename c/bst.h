#ifndef BST_H
#define BST_H

#include <time.h>

typedef struct Vehicle
{
    char number[20];
    char type[20];

    int lot;
    int slot;
    int vip;

    time_t entryTime;

    struct Vehicle *left;
    struct Vehicle *right;

} Vehicle;

Vehicle* insertVehicle(Vehicle *root, Vehicle *newVehicle);

Vehicle* searchVehicle(Vehicle *root, char number[]);

Vehicle* deleteVehicle(Vehicle *root, char number[]);

void displayVehicles(Vehicle *root);

void freeTree(Vehicle *root);

#endif