#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bst.h"

Vehicle* insertVehicle(Vehicle *root, Vehicle *newVehicle)
{
    if(root == NULL)
        return newVehicle;

    if(strcmp(newVehicle->number, root->number) < 0)
    {
        root->left =
            insertVehicle(root->left, newVehicle);
    }
    else if(strcmp(newVehicle->number, root->number) > 0)
    {
        root->right =
            insertVehicle(root->right, newVehicle);
    }

    return root;
}

Vehicle* searchVehicle(Vehicle *root, char number[])
{
    if(root == NULL)
        return NULL;

    int result = strcmp(number, root->number);

    if(result == 0)
        return root;

    if(result < 0)
        return searchVehicle(root->left, number);

    return searchVehicle(root->right, number);
}

Vehicle* findMinimum(Vehicle *root)
{
    Vehicle *current = root;

    while(current != NULL && current->left != NULL)
        current = current->left;

    return current;
}

Vehicle* deleteVehicle(Vehicle *root, char number[])
{
    if(root == NULL)
        return NULL;

    int result = strcmp(number, root->number);

    if(result < 0)
    {
        root->left =
            deleteVehicle(root->left, number);
    }
    else if(result > 0)
    {
        root->right =
            deleteVehicle(root->right, number);
    }
    else
    {
        if(root->left == NULL)
        {
            Vehicle *temp = root->right;
            free(root);
            return temp;
        }

        if(root->right == NULL)
        {
            Vehicle *temp = root->left;
            free(root);
            return temp;
        }

        Vehicle *temp = findMinimum(root->right);

        strcpy(root->number, temp->number);
        strcpy(root->type, temp->type);

        root->lot = temp->lot;
        root->slot = temp->slot;
        root->vip = temp->vip;
        root->entryTime = temp->entryTime;

        root->right =
            deleteVehicle(root->right, temp->number);
    }

    return root;
}

void displayVehicles(Vehicle *root)
{
    if(root == NULL)
        return;

    displayVehicles(root->left);

    printf("%s | %s | Lot %d | Slot %d\n",
           root->number,
           root->type,
           root->lot + 1,
           root->slot + 1);

    displayVehicles(root->right);
}

void freeTree(Vehicle *root)
{
    if(root == NULL)
        return;

    freeTree(root->left);
    freeTree(root->right);

    free(root);
}