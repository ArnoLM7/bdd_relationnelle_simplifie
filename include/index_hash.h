#ifndef INDEX_HASH_H
#define INDEX_HASH_H

#include <stddef.h>
#include <stdint.h>

#include "idlist.h"
#include "value.h"

typedef struct HashEntry {
    Value key;
    uint64_t hash;
    IdList ids;
    struct HashEntry *next;
} HashEntry;

typedef struct {
    HashEntry **buckets;
    size_t nbuckets;
    size_t nkeys;
} HashIndex;

HashIndex *hash_create(void);
void hash_free(HashIndex *h);
int hash_insert(HashIndex *h, const Value *key, size_t row_id);
const IdList *hash_find(const HashIndex *h, const Value *key);

#endif
