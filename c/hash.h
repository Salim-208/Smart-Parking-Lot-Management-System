#ifndef HASH_H
#define HASH_H

#define HASH_SIZE 101

typedef struct
{
    char number[20];

    int lot;
    int slot;

    int used;

} HashEntry;

typedef struct
{
    HashEntry table[HASH_SIZE];

} HashTable;

void initializeHash(HashTable *ht);

int insertHash(HashTable *ht,
               char number[],
               int lot,
               int slot);

int searchHash(HashTable *ht,
               char number[],
               int *lot,
               int *slot);

int deleteHash(HashTable *ht,
               char number[]);

#endif