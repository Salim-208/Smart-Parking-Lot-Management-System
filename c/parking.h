#ifndef PARKING_H
#define PARKING_H

#define LOTS 3
#define SLOTS 10
#define VIP_SLOTS 3

extern int parking[LOTS][SLOTS];

void initializeParking();
void showParking();

#endif