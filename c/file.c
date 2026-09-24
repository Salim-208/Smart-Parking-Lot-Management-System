#include <stdio.h>

#include "file.h"

void saveInorder(Vehicle *root,
                 FILE *file)
{
    if(root == NULL)
        return;

    saveInorder(root->left, file);

    fprintf(file,
            "%s,%s,%d,%d,%d,%lld\n",
            root->number,
            root->type,
            root->lot + 1,
            root->slot + 1,
            root->vip,
            (long long)root->entryTime);

    saveInorder(root->right, file);
}

void saveRecords(Vehicle *root)
{
    FILE *file =
        fopen("parking_records.txt", "w");

    if(file == NULL)
    {
        printf("File cannot be opened.\n");
        return;
    }

    fprintf(file,
            "Vehicle,Type,Lot,Slot,VIP,EntryTime\n");

    saveInorder(root, file);

    fclose(file);

    printf("Records saved successfully.\n");
}