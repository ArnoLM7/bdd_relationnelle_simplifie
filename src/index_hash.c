#include "index_hash.h"

#include <stdlib.h>

#define HASH_INITIAL_BUCKETS 16

HashIndex *hash_create(void)
{
    HashIndex *h = malloc(sizeof(*h));
    if (h == NULL)
        return NULL;

    h->buckets = calloc(HASH_INITIAL_BUCKETS, sizeof(HashEntry *));
    if (h->buckets == NULL) {
        free(h);
        return NULL;
    }
    h->nbuckets = HASH_INITIAL_BUCKETS;
    h->nkeys = 0;
    return h;
}

void hash_free(HashIndex *h)
{
    if (h == NULL)
        return;
    for (size_t b = 0; b < h->nbuckets; b++) {
        HashEntry *e = h->buckets[b];
        while (e != NULL) {
            HashEntry *next = e->next;
            value_release(&e->key);
            idlist_free(&e->ids);
            free(e);
            e = next;
        }
    }
    free(h->buckets);
    free(h);
}

static HashEntry *find_entry(const HashIndex *h, const Value *key, uint64_t hv)
{
    HashEntry *e = h->buckets[hv & (h->nbuckets - 1)];
    while (e != NULL) {
        if (e->hash == hv && value_compare(&e->key, key) == 0)
            return e;
        e = e->next;
    }
    return NULL;
}

static int hash_grow(HashIndex *h)
{
    size_t new_n = h->nbuckets * 2;
    HashEntry **nb = calloc(new_n, sizeof(HashEntry *));
    if (nb == NULL)
        return -1;

    for (size_t b = 0; b < h->nbuckets; b++) {
        HashEntry *e = h->buckets[b];
        while (e != NULL) {
            HashEntry *next = e->next;
            size_t slot = e->hash & (new_n - 1);
            e->next = nb[slot];
            nb[slot] = e;
            e = next;
        }
    }

    free(h->buckets);
    h->buckets = nb;
    h->nbuckets = new_n;
    return 0;
}

int hash_insert(HashIndex *h, const Value *key, size_t row_id)
{
    uint64_t hv = value_hash(key);
    HashEntry *e = find_entry(h, key, hv);
    if (e != NULL)
        return idlist_push(&e->ids, row_id);

    if ((h->nkeys + 1) * 4 > h->nbuckets * 3)
        hash_grow(h);

    e = calloc(1, sizeof(*e));
    if (e == NULL)
        return -1;
    if (value_dup(&e->key, key) != 0) {
        free(e);
        return -1;
    }
    if (idlist_push(&e->ids, row_id) != 0) {
        value_release(&e->key);
        free(e);
        return -1;
    }

    e->hash = hv;
    size_t slot = hv & (h->nbuckets - 1);
    e->next = h->buckets[slot];
    h->buckets[slot] = e;
    h->nkeys++;
    return 0;
}

const IdList *hash_find(const HashIndex *h, const Value *key)
{
    HashEntry *e = find_entry(h, key, value_hash(key));
    return e ? &e->ids : NULL;
}
