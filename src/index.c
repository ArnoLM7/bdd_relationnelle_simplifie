#include "index.h"

#include <stdlib.h>

static Index *index_new(size_t col, IndexKind kind)
{
    Index *idx = malloc(sizeof(*idx));
    if (idx == NULL)
        return NULL;

    idx->col = col;
    idx->kind = kind;
    if (kind == INDEX_HASH)
        idx->impl.hash = hash_create();
    else
        idx->impl.avl = avl_create();

    if ((kind == INDEX_HASH && idx->impl.hash == NULL) ||
        (kind == INDEX_AVL && idx->impl.avl == NULL)) {
        free(idx);
        return NULL;
    }
    return idx;
}

void index_free(Index *idx)
{
    if (idx == NULL)
        return;
    if (idx->kind == INDEX_HASH)
        hash_free(idx->impl.hash);
    else
        avl_free(idx->impl.avl);
    free(idx);
}

int index_insert(Index *idx, const Value *key, size_t row_id)
{
    if (idx->kind == INDEX_HASH)
        return hash_insert(idx->impl.hash, key, row_id);
    return avl_insert(idx->impl.avl, key, row_id);
}

const Index *table_find_index(const Table *t, size_t col, IndexKind kind)
{
    for (size_t i = 0; i < t->nindexes; i++) {
        if (t->indexes[i]->col == col && t->indexes[i]->kind == kind)
            return t->indexes[i];
    }
    return NULL;
}

int create_index(Table *t, const char *col, IndexKind kind)
{
    if (t == NULL || col == NULL)
        return -1;
    if (kind != INDEX_HASH && kind != INDEX_AVL)
        return -1;

    int c = schema_find_column(t->schema, col);
    if (c < 0)
        return -1;
    if (table_find_index(t, (size_t)c, kind) != NULL)
        return -1;

    Index *idx = index_new((size_t)c, kind);
    if (idx == NULL)
        return -1;

    size_t n = table_row_count(t);
    for (size_t id = 0; id < n; id++) {
        Value key = row_get_value(t, get_row(t, id), (size_t)c);
        if (index_insert(idx, &key, id) != 0) {
            index_free(idx);
            return -1;
        }
    }

    Index **tmp = realloc(t->indexes, (t->nindexes + 1) * sizeof(Index *));
    if (tmp == NULL) {
        index_free(idx);
        return -1;
    }
    t->indexes = tmp;
    t->indexes[t->nindexes++] = idx;
    return 0;
}

const char *index_kind_name(IndexKind kind)
{
    return kind == INDEX_HASH ? "HASH" : "AVL";
}
