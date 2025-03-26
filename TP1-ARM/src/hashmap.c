#include "hashmap.h"

static int hash_function(HashMap *map, uint32_t mask, uint32_t masked_pattern) {
    return (mask ^ masked_pattern) % map->size;
}

HashMap* hashmap_create() {
    HashMap *map = (HashMap*) malloc(sizeof(HashMap));
    if (!map) return NULL;
    
    map->size = 101; // Número primo para mejor distribución
    map->buckets = (HashEntry**) calloc(map->size, sizeof(HashEntry*));
    
    return map;
}

void hashmap_put(HashMap *map, uint32_t mask, uint32_t masked_pattern, void *value) {
    int bucket = hash_function(map, mask, masked_pattern);
    
    HashEntry *entry = map->buckets[bucket];
    while (entry) {
        if (entry->mask == mask && entry->masked_pattern == masked_pattern) {
            entry->value = value; 
            return;
        }
        entry = entry->next;
    }
    
    HashEntry *new_entry = (HashEntry*) malloc(sizeof(HashEntry));
    if (!new_entry) return;
    
    new_entry->mask = mask;
    new_entry->masked_pattern = masked_pattern;
    new_entry->value = value;
    new_entry->next = map->buckets[bucket];
    map->buckets[bucket] = new_entry;
}

void* hashmap_get(HashMap *map, uint32_t mask, uint32_t masked_pattern) {
    int bucket = hash_function(map, mask, masked_pattern);
    
    HashEntry *entry = map->buckets[bucket];
    while (entry) {
        if (entry->mask == mask && entry->masked_pattern == masked_pattern) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}

void hashmap_free(HashMap *map) {
    if (!map) return;
    
    for (int i = 0; i < map->size; i++) {
        HashEntry *entry = map->buckets[i];
        while (entry) {
            HashEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    
    free(map->buckets);
    free(map);
}
