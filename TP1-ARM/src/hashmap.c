#include "hashmap.h"

static inline int hash_function(const HashMap *map, uint32_t length, uint32_t opcode) {
    return ((length * 31) ^ opcode) % map->size;
}

HashMap *hashmap_create(void) {
    HashMap *map = malloc(sizeof(*map));
    if (!map) return NULL;
    map->size = 101;
    map->buckets = calloc(map->size, sizeof(HashEntry *));
    return map;
}

void hashmap_put(HashMap *map, uint32_t length, uint32_t opcode, void *value) {
    int bucket = hash_function(map, length, opcode);
    HashEntry *entry = map->buckets[bucket];
    while (entry) {
        if (entry->length == length && entry->opcode == opcode) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }
    HashEntry *new_entry = malloc(sizeof(*new_entry));
    if (!new_entry) return;
    new_entry->length = length;
    new_entry->opcode = opcode;
    new_entry->value = value;
    new_entry->next = map->buckets[bucket];
    map->buckets[bucket] = new_entry;
}

void *hashmap_get(HashMap *map, uint32_t length, uint32_t opcode) {
    int bucket = hash_function(map, length, opcode);
    for (HashEntry *entry = map->buckets[bucket]; entry; entry = entry->next)
        if (entry->length == length && entry->opcode == opcode)
            return entry->value;
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
