#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "query.h"
#include "table.h"

#define N 5000

static ColumnDef cols[] = {
    { "id",    COL_INT,   0 },
    { "age",   COL_INT,   0 },
    { "score", COL_FLOAT, 0 },
    { "city",  COL_TEXT,  15 },
};

static const char *cities[] = { "Paris", "Lyon", "Lille", "Nantes" };

static Table *make_table(void)
{
    Table *t = create_table_struct("people", cols, 4);
    assert(t != NULL);
    for (int i = 0; i < N; i++) {
        Value v[] = {
            val_int(i),
            val_int(18 + (i * 7) % 40),
            val_float((double)((i * 13) % 21)),
            val_text(cities[i % 4]),
        };
        assert(insert_row(t, v) == i);
    }
    return t;
}

static int cmp_size(const void *a, const void *b)
{
    size_t x = *(const size_t *)a;
    size_t y = *(const size_t *)b;
    return (x > y) - (x < y);
}

static void assert_same(ResultSet *a, ResultSet *b)
{
    assert(a != NULL && b != NULL);
    assert(a->count == b->count);
    if (a->count == 0)
        return;
    qsort(a->row_ids, a->count, sizeof(size_t), cmp_size);
    qsort(b->row_ids, b->count, sizeof(size_t), cmp_size);
    for (size_t i = 0; i < a->count; i++)
        assert(a->row_ids[i] == b->row_ids[i]);
}

static void test_hash_index(void)
{
    HashIndex *h = hash_create();
    assert(h != NULL);

    for (int i = 0; i < 20000; i++) {
        Value k = val_int(i);
        assert(hash_insert(h, &k, (size_t)i) == 0);
    }
    Value dup = val_int(42);
    assert(hash_insert(h, &dup, 99999) == 0);

    assert(h->nkeys == 20000);
    assert((h->nbuckets & (h->nbuckets - 1)) == 0);
    assert(h->nkeys * 4 <= h->nbuckets * 3);

    const IdList *l = hash_find(h, &dup);
    assert(l != NULL && l->count == 2);
    assert(l->ids[0] == 42 && l->ids[1] == 99999);

    Value k = val_int(12345);
    l = hash_find(h, &k);
    assert(l != NULL && l->count == 1 && l->ids[0] == 12345);

    Value missing = val_int(-1);
    assert(hash_find(h, &missing) == NULL);
    hash_free(h);

    h = hash_create();
    char buf[16];
    for (int i = 0; i < 1000; i++) {
        snprintf(buf, sizeof(buf), "key%d", i);
        Value s = val_text(buf);
        assert(hash_insert(h, &s, (size_t)i) == 0);
    }
    Value s = val_text("key777");
    l = hash_find(h, &s);
    assert(l != NULL && l->count == 1 && l->ids[0] == 777);
    hash_free(h);
}

static int check_avl(const AvlNode *n, const Value *lo, const Value *hi)
{
    if (n == NULL)
        return 0;
    if (lo)
        assert(value_compare(&n->key, lo) > 0);
    if (hi)
        assert(value_compare(&n->key, hi) < 0);
    int hl = check_avl(n->left, lo, &n->key);
    int hr = check_avl(n->right, &n->key, hi);
    assert(abs(hl - hr) <= 1);
    assert(n->height == 1 + (hl > hr ? hl : hr));
    return n->height;
}

static void test_avl_index(void)
{
    AvlIndex *a = avl_create();
    const int n = 100000;

    for (int i = 1; i <= n; i++) {
        Value k = val_int(i);
        assert(avl_insert(a, &k, (size_t)i) == 0);
    }
    assert(a->nkeys == (size_t)n);
    check_avl(a->root, NULL, NULL);

    int lg = 0;
    while ((1 << lg) < n)
        lg++;
    assert(avl_height(a) <= lg * 3 / 2 + 2);

    Value k = val_int(500);
    const IdList *l = avl_find(a, &k);
    assert(l != NULL && l->ids[0] == 500);

    Value lo = val_int(100), hi = val_int(200);
    IdList out = { NULL, 0, 0 };
    assert(avl_range(a, &lo, 1, &hi, 1, &out) == 0);
    assert(out.count == 101);
    for (size_t i = 0; i < out.count; i++)
        assert(out.ids[i] == 100 + i);
    idlist_free(&out);

    assert(avl_range(a, &lo, 0, &hi, 0, &out) == 0);
    assert(out.count == 99);
    idlist_free(&out);

    Value ten = val_int(10);
    assert(avl_range(a, NULL, 0, &ten, 0, &out) == 0);
    assert(out.count == 9);
    idlist_free(&out);

    Value big = val_int(n - 4);
    assert(avl_range(a, &big, 1, NULL, 0, &out) == 0);
    assert(out.count == 5);
    idlist_free(&out);

    avl_free(a);
}

static void check_query(const Table *plain, const Table *indexed, const Predicate *p,
                        size_t n, PlanKind expected)
{
    ResultSet *a = select_values(plain, p, n, 0);
    ResultSet *b = select_values(indexed, p, n, 0);
    assert(a->plan == PLAN_SCAN);
    assert(b->plan == expected);
    if (expected != PLAN_SCAN)
        assert(b->examined <= a->examined);
    assert_same(a, b);
    free_result(a);
    free_result(b);
}

static void test_index_matches_scan(void)
{
    Table *plain = make_table();
    Table *indexed = make_table();

    assert(create_index(indexed, "id", INDEX_HASH) == 0);
    assert(create_index(indexed, "age", INDEX_AVL) == 0);
    assert(create_index(indexed, "city", INDEX_HASH) == 0);
    assert(create_index(indexed, "score", INDEX_AVL) == 0);

    Predicate p1[] = { pred("id", OP_EQ, val_int(1234)) };
    check_query(plain, indexed, p1, 1, PLAN_HASH);

    Predicate p2[] = { pred("age", OP_EQ, val_int(30)) };
    check_query(plain, indexed, p2, 1, PLAN_AVL);

    Op ops[] = { OP_LT, OP_LE, OP_GT, OP_GE };
    for (size_t i = 0; i < 4; i++) {
        Predicate p[] = { pred("age", ops[i], val_int(40)) };
        check_query(plain, indexed, p, 1, PLAN_AVL);
    }

    Predicate p3[] = { pred_between("age", val_int(25), val_int(30)) };
    check_query(plain, indexed, p3, 1, PLAN_AVL);

    Predicate p4[] = { pred("city", OP_EQ, val_text("Lille")) };
    check_query(plain, indexed, p4, 1, PLAN_HASH);

    Predicate p5[] = { pred_between("score", val_float(5.0), val_float(9.5)) };
    check_query(plain, indexed, p5, 1, PLAN_AVL);

    Predicate p6[] = {
        pred("age", OP_GE, val_int(50)),
        pred("city", OP_EQ, val_text("Lyon")),
        pred("score", OP_LT, val_float(10.0)),
    };
    check_query(plain, indexed, p6, 3, PLAN_HASH);

    Predicate p7[] = {
        pred("age", OP_GT, val_int(20)),
        pred_between("age", val_int(30), val_int(35)),
    };
    check_query(plain, indexed, p7, 2, PLAN_AVL);

    Predicate p8[] = { pred("age", OP_NE, val_int(30)) };
    check_query(plain, indexed, p8, 1, PLAN_SCAN);

    Predicate p9[] = { pred("id", OP_EQ, val_int(-5)) };
    check_query(plain, indexed, p9, 1, PLAN_HASH);

    ResultSet *rs = select_values(indexed, p1, 1, 0);
    assert(rs->count == 1 && rs->row_ids[0] == 1234 && rs->examined == 1);
    free_result(rs);

    rs = select_values(indexed, p4, 1, 3);
    assert(rs->count == 3 && rs->examined == 3);
    free_result(rs);

    free_table(plain);
    free_table(indexed);
}

static void test_index_maintained(void)
{
    Table *t = create_table_struct("people", cols, 4);
    assert(create_index(t, "id", INDEX_HASH) == 0);
    assert(create_index(t, "age", INDEX_AVL) == 0);

    for (int i = 0; i < 1000; i++) {
        Value v[] = { val_int(i), val_int(i % 50), val_float(0.0), val_text("Paris") };
        assert(insert_row(t, v) == i);
    }

    Predicate p = pred("id", OP_EQ, val_int(999));
    ResultSet *rs = select_values(t, &p, 1, 0);
    assert(rs->plan == PLAN_HASH && rs->count == 1 && rs->row_ids[0] == 999);
    free_result(rs);

    p = pred("age", OP_EQ, val_int(7));
    rs = select_values(t, &p, 1, 0);
    assert(rs->plan == PLAN_AVL && rs->count == 20);
    free_result(rs);

    free_table(t);
}

static void test_create_index_errors(void)
{
    Table *t = make_table();
    assert(create_index(t, "nope", INDEX_HASH) == -1);
    assert(create_index(t, "age", INDEX_HASH) == 0);
    assert(create_index(t, "age", INDEX_HASH) == -1);
    assert(create_index(t, "age", INDEX_AVL) == 0);
    assert(t->nindexes == 2);
    assert(create_index(NULL, "age", INDEX_AVL) == -1);
    free_table(t);
}

int main(void)
{
    test_hash_index();
    test_avl_index();
    test_index_matches_scan();
    test_index_maintained();
    test_create_index_errors();
    printf("test_index : OK\n");
    return 0;
}
