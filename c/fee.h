#ifndef FEE_H
#define FEE_H

#include "bst.h"

double getRate(char type[]);

double calculateFee(Vehicle *vehicle);

void printBill(Vehicle *vehicle);

#endif
