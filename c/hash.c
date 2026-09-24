#include <string.h>

#include "hash.h"

unsigned long hashFunction(char str[])
{
    unsigned long hash = 5381;

    int c;

    while((c = *str++))
    {
        hash =
            ((hash << 5) + hash) + c;
    }

    return hash % HASH_SIZE;
}

void initializeHash(HashTable *ht)
{
    for(int i = 0; i < HASH_SIZE; i++)
        ht->table[i].used = 0;
}

int insertHash(HashTable *ht,
               char number[],
               int lot,
               int slot)
{
    unsigned long index =
        hashFunction(number);

    for(int i = 0; i < HASH_SIZE; i++)
    {
        int position =
            (index + i) % HASH_SIZE;

        if(!ht->table[position].used)
        {
            strcpy(ht->table[position].number,
                   number);

            ht->table[position].lot = lot;
            ht->table[position].slot = slot;
            ht->table[position].used = 1;

            return 1;
        }
    }

    return 0;
}

int searchHash(HashTable *ht,
               char number[],
               int *lot,
               int *slot)
{
    unsigned long index =
        hashFunction(number);

    for(int i = 0; i < HASH_SIZE; i++)
    {
        int position =
            (index + i) % HASH_SIZE;

        if(!ht->table[position].used)
            return 0;

        if(strcmp(ht->table[position].number,
                  number) == 0)
        {
            *lot =
                ht->table[position].lot;

            *slot =
                ht->table[position].slot;

            return 1;
        }
    }

    return 0;
}

int deleteHash(HashTable *ht,
               char number[])
{
    unsigned long index =
        hashFunction(number);

    for(int i = 0; i < HASH_SIZE; i++)
    {
        int position =
            (index + i) % HASH_SIZE;

        if(!ht->table[position].used)
            return 0;

        if(strcmp(ht->table[position].number,
                  number) == 0)
        {
            ht->table[position].used = 0;
            return 1;
        }
    }

    return 0;
}