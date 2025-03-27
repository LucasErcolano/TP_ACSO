#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdlib.h>

typedef struct HashEntry {
    uint32_t length; 
    uint32_t opcode;
    void *value;
    struct HashEntry *next;
} HashEntry;

typedef struct HashMap {
    HashEntry **buckets;
    int size;
} HashMap;

HashMap *hashmap_create(void);
void hashmap_put(HashMap *map, uint32_t length, uint32_t opcode, void *value);
void *hashmap_get(HashMap *map, uint32_t length, uint32_t opcode);
void hashmap_free(HashMap *map);

#endif
