#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "query.h"
#include "table.h"

#define N 1000

static ColumnDef cols[] = {
    { "id",    COL_INT,   0 },
    { "age",   COL_INT,   0 },
    { "score", COL_FLOAT, 0 },
    { "city",  COL_TEXT,  15 },
};

static const char *cities[] = { "Paris", "Lyon", "Lille", "Nantes" };

static int age_of(int i)
{
    return 18 + (i * 7) % 40;
}

static double score_of(int i)
{
    return (double)((i * 13) % 21);
}

static Table *make_table(void)
{
    Table *t = create_table_struct("people", cols, 4);
    assert(t != NULL);
    for (int i = 0; i < N; i++) {
        Value v[] = { val_int(i), val_int(age_of(i)), val_float(score_of(i)), val_text(cities[i % 4]) };
        assert(insert_row(t, v) == i);
    }
    return t;
}

static size_t count_if(int (*f)(int))
{
    size_t c = 0;
    for (int i = 0; i < N; i++)
        c += (size_t)f(i);
    return c;
}

static int is_age_eq_30(int i) { return age_of(i) == 30; }
static int is_age_ne_30(int i) { return age_of(i) != 30; }
static int is_age_lt_25(int i) { return age_of(i) < 25; }
static int is_age_le_25(int i) { return age_of(i) <= 25; }
static int is_age_gt_50(int i) { return age_of(i) > 50; }
static int is_age_ge_50(int i) { return age_of(i) >= 50; }
static int is_age_20_29(int i) { return age_of(i) >= 20 && age_of(i) <= 29; }
static int is_lyon_and_good(int i) { return i % 4 == 1 && score_of(i) >= 15.0 && age_of(i) < 40; }

static void check(const Table *t, Predicate p, int (*f)(int))
{
    ResultSet *rs = select_values(t, &p, 1, 0);
    assert(rs != NULL);
    assert(rs->count == count_if(f));
    for (size_t k = 0; k < rs->count; k++)
        assert(f((int)rs->row_ids[k]));
    free_result(rs);
}

static void test_operators(void)
{
    Table *t = make_table();
    check(t, pred("age", OP_EQ, val_int(30)), is_age_eq_30);
    check(t, pred("age", OP_NE, val_int(30)), is_age_ne_30);
    check(t, pred("age", OP_LT, val_int(25)), is_age_lt_25);
    check(t, pred("age", OP_LE, val_int(25)), is_age_le_25);
    check(t, pred("age", OP_GT, val_int(50)), is_age_gt_50);
    check(t, pred("age", OP_GE, val_int(50)), is_age_ge_50);
    check(t, pred_between("age", val_int(20), val_int(29)), is_age_20_29);
    free_table(t);
}

static void test_text(void)
{
    Table *t = make_table();
    Predicate p = pred("city", OP_EQ, val_text("Lille"));
    ResultSet *rs = select_values(t, &p, 1, 0);
    assert(rs != NULL);
    assert(rs->count == N / 4);
    for (size_t k = 0; k < rs->count; k++)
        assert(rs->row_ids[k] % 4 == 2);
    free_result(rs);

    p = pred("city", OP_LT, val_text("M"));
    rs = select_values(t, &p, 1, 0);
    assert(rs->count == N / 2);
    free_result(rs);
    free_table(t);
}

static void test_and(void)
{
    Table *t = make_table();
    Predicate p[] = {
        pred("city", OP_EQ, val_text("Lyon")),
        pred("score", OP_GE, val_float(15.0)),
        pred("age", OP_LT, val_int(40)),
    };
    ResultSet *rs = select_values(t, p, 3, 0);
    assert(rs != NULL);
    assert(rs->count == count_if(is_lyon_and_good));
    for (size_t k = 0; k < rs->count; k++)
        assert(is_lyon_and_good((int)rs->row_ids[k]));
    free_result(rs);
    free_table(t);
}

static void test_no_predicate_and_limit(void)
{
    Table *t = make_table();

    ResultSet *rs = select_values(t, NULL, 0, 0);
    assert(rs != NULL && rs->count == N);
    for (size_t k = 0; k < rs->count; k++)
        assert(rs->row_ids[k] == k);
    free_result(rs);

    rs = select_values(t, NULL, 0, 10);
    assert(rs->count == 10);
    free_result(rs);

    Predicate p = pred("age", OP_GE, val_int(50));
    rs = select_values(t, &p, 1, 5);
    assert(rs->count == 5);
    for (size_t k = 0; k < rs->count; k++)
        assert(age_of((int)rs->row_ids[k]) >= 50);
    for (size_t k = 1; k < rs->count; k++)
        assert(rs->row_ids[k] > rs->row_ids[k - 1]);
    free_result(rs);

    free_table(t);
}

static void test_empty_and_invalid(void)
{
    Table *t = make_table();

    Predicate none = pred("age", OP_GT, val_int(1000));
    ResultSet *rs = select_values(t, &none, 1, 0);
    assert(rs != NULL && rs->count == 0);
    free_result(rs);

    Predicate unknown = pred("ville", OP_EQ, val_text("Paris"));
    assert(select_values(t, &unknown, 1, 0) == NULL);

    Predicate wrong_type = pred("age", OP_EQ, val_text("30"));
    assert(select_values(t, &wrong_type, 1, 0) == NULL);

    Predicate null_text = pred("city", OP_EQ, val_text(NULL));
    assert(select_values(t, &null_text, 1, 0) == NULL);

    Predicate bad_between = pred_between("age", val_int(20), val_float(30.0));
    assert(select_values(t, &bad_between, 1, 0) == NULL);

    assert(select_values(NULL, NULL, 0, 0) == NULL);
    assert(select_values(t, NULL, 2, 0) == NULL);

    free_table(t);
}

static void test_empty_table(void)
{
    Table *t = create_table_struct("empty", cols, 4);
    Predicate p = pred("age", OP_GT, val_int(0));
    ResultSet *rs = select_values(t, &p, 1, 0);
    assert(rs != NULL && rs->count == 0);
    free_result(rs);
    free_table(t);
}

int main(void)
{
    test_operators();
    test_text();
    test_and();
    test_no_predicate_and_limit();
    test_empty_and_invalid();
    test_empty_table();
    printf("test_query : OK\n");
    return 0;
}
