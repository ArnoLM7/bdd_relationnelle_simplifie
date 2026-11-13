#include "index_avl.h"

#include <stdlib.h>

AvlIndex *avl_create(void)
{
    AvlIndex *a = malloc(sizeof(*a));
    if (a == NULL)
        return NULL;
    a->root = NULL;
    a->nkeys = 0;
    return a;
}

static void free_node(AvlNode *n)
{
    if (n == NULL)
        return;
    free_node(n->left);
    free_node(n->right);
    value_release(&n->key);
    idlist_free(&n->ids);
    free(n);
}

void avl_free(AvlIndex *a)
{
    if (a == NULL)
        return;
    free_node(a->root);
    free(a);
}

static int height(const AvlNode *n)
{
    return n ? n->height : 0;
}

static void update_height(AvlNode *n)
{
    int hl = height(n->left);
    int hr = height(n->right);
    n->height = 1 + (hl > hr ? hl : hr);
}

static AvlNode *rotate_right(AvlNode *y)
{
    AvlNode *x = y->left;
    y->left = x->right;
    x->right = y;
    update_height(y);
    update_height(x);
    return x;
}

static AvlNode *rotate_left(AvlNode *x)
{
    AvlNode *y = x->right;
    x->right = y->left;
    y->left = x;
    update_height(x);
    update_height(y);
    return y;
}

static AvlNode *rebalance(AvlNode *n)
{
    update_height(n);
    int balance = height(n->left) - height(n->right);

    if (balance > 1) {
        if (height(n->left->left) < height(n->left->right))
            n->left = rotate_left(n->left);
        return rotate_right(n);
    }
    if (balance < -1) {
        if (height(n->right->right) < height(n->right->left))
            n->right = rotate_right(n->right);
        return rotate_left(n);
    }
    return n;
}

static AvlNode *new_node(const Value *key, size_t row_id)
{
    AvlNode *n = calloc(1, sizeof(*n));
    if (n == NULL)
        return NULL;
    if (value_dup(&n->key, key) != 0) {
        free(n);
        return NULL;
    }
    if (idlist_push(&n->ids, row_id) != 0) {
        value_release(&n->key);
        free(n);
        return NULL;
    }
    n->height = 1;
    return n;
}

static AvlNode *insert_rec(AvlIndex *a, AvlNode *n, const Value *key, size_t row_id, int *err)
{
    if (n == NULL) {
        AvlNode *created = new_node(key, row_id);
        if (created == NULL)
            *err = 1;
        else
            a->nkeys++;
        return created;
    }

    int c = value_compare(key, &n->key);
    if (c == 0) {
        if (idlist_push(&n->ids, row_id) != 0)
            *err = 1;
        return n;
    }

    AvlNode *child;
    if (c < 0) {
        child = insert_rec(a, n->left, key, row_id, err);
        if (*err)
            return n;
        n->left = child;
    } else {
        child = insert_rec(a, n->right, key, row_id, err);
        if (*err)
            return n;
        n->right = child;
    }
    return rebalance(n);
}

int avl_insert(AvlIndex *a, const Value *key, size_t row_id)
{
    int err = 0;
    AvlNode *root = insert_rec(a, a->root, key, row_id, &err);
    if (err)
        return -1;
    a->root = root;
    return 0;
}

const IdList *avl_find(const AvlIndex *a, const Value *key)
{
    const AvlNode *n = a->root;
    while (n != NULL) {
        int c = value_compare(key, &n->key);
        if (c == 0)
            return &n->ids;
        n = c < 0 ? n->left : n->right;
    }
    return NULL;
}

static int range_rec(const AvlNode *n, const Value *low, int low_incl,
                     const Value *high, int high_incl, IdList *out)
{
    if (n == NULL)
        return 0;

    int cl = low ? value_compare(&n->key, low) : 1;
    int ch = high ? value_compare(&n->key, high) : -1;

    if (cl > 0 && range_rec(n->left, low, low_incl, high, high_incl, out) != 0)
        return -1;

    int above = low_incl ? cl >= 0 : cl > 0;
    int below = high_incl ? ch <= 0 : ch < 0;
    if (above && below) {
        for (size_t i = 0; i < n->ids.count; i++) {
            if (idlist_push(out, n->ids.ids[i]) != 0)
                return -1;
        }
    }

    if (ch < 0 && range_rec(n->right, low, low_incl, high, high_incl, out) != 0)
        return -1;
    return 0;
}

int avl_range(const AvlIndex *a, const Value *low, int low_incl,
              const Value *high, int high_incl, IdList *out)
{
    return range_rec(a->root, low, low_incl, high, high_incl, out);
}

int avl_height(const AvlIndex *a)
{
    return height(a->root);
}
