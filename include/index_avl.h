#ifndef INDEX_AVL_H
#define INDEX_AVL_H

#include <stddef.h>

#include "idlist.h"
#include "value.h"

typedef struct AvlNode {
    Value key;
    IdList ids;
    struct AvlNode *left;
    struct AvlNode *right;
    int height;
} AvlNode;

typedef struct {
    AvlNode *root;
    size_t nkeys;
} AvlIndex;

AvlIndex *avl_create(void);
void avl_free(AvlIndex *a);
int avl_insert(AvlIndex *a, const Value *key, size_t row_id);
const IdList *avl_find(const AvlIndex *a, const Value *key);
int avl_range(const AvlIndex *a, const Value *low, int low_incl,
              const Value *high, int high_incl, IdList *out);
int avl_height(const AvlIndex *a);

#endif
