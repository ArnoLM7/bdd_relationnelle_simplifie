#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>

#include "index_avl.h"
#include "index_hash.h"
#include "table.h"

typedef enum {
    INDEX_HASH,
    INDEX_AVL
} IndexKind;

typedef struct Index {
    size_t col;
    IndexKind kind;
    union {
        HashIndex *hash;
        AvlIndex *avl;
    } impl;
} Index;

int create_index(Table *t, const char *col, IndexKind kind);
const Index *table_find_index(const Table *t, size_t col, IndexKind kind);
int index_insert(Index *idx, const Value *key, size_t row_id);
void index_free(Index *idx);
const char *index_kind_name(IndexKind kind);

#endif
