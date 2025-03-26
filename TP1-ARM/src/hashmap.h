#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdlib.h>

typedef struct HashEntry {
    uint32_t mask;
    uint32_t masked_pattern;
    void *value;
    struct HashEntry *next;
} HashEntry;

typedef struct HashMap {
    HashEntry **buckets;
    int size;
} HashMap;

HashMap* hashmap_create();
void hashmap_put(HashMap *map, uint32_t mask, uint32_t masked_pattern, void *value);
void* hashmap_get(HashMap *map, uint32_t mask, uint32_t masked_pattern);
void hashmap_free(HashMap *map);

#endif
