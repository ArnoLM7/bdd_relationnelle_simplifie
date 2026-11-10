#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "table.h"

static ColumnDef users_cols[] = {
    { "id",    COL_INT,   0 },
    { "age",   COL_INT,   0 },
    { "score", COL_FLOAT, 0 },
    { "name",  COL_TEXT,  31 },
};

static void test_schema_offsets(void)
{
    Table *t = create_table_struct("users", users_cols, 4);
    assert(t != NULL);

    const Schema *s = t->schema;
    assert(s->ncols == 4);
    assert(s->cols[0].offset == 0);
    assert(s->cols[1].offset == 4);
    assert(s->cols[2].offset == 8);
    assert(s->cols[3].offset == 16);
    assert(s->cols[3].size == 32);
    assert(s->row_size == 48);

    assert(schema_find_column(s, "age") == 1);
    assert(schema_find_column(s, "name") == 3);
    assert(schema_find_column(s, "nope") == -1);

    free_table(t);
}

static void test_padding(void)
{
    ColumnDef cols[] = {
        { "flag",  COL_INT,   0 },
        { "value", COL_FLOAT, 0 },
        { "tag",   COL_TEXT,  2 },
    };
    Table *t = create_table_struct("pad", cols, 3);
    assert(t != NULL);
    assert(t->schema->cols[1].offset == 8);
    assert(t->schema->cols[2].offset == 16);
    assert(t->schema->row_size == 24);
    free_table(t);
}

static void test_invalid_schema(void)
{
    ColumnDef dup[] = {
        { "id", COL_INT, 0 },
        { "id", COL_INT, 0 },
    };
    assert(create_table_struct("t", dup, 2) == NULL);

    ColumnDef bad_text[] = { { "name", COL_TEXT, 0 } };
    assert(create_table_struct("t", bad_text, 1) == NULL);

    assert(create_table_struct("t", users_cols, 0) == NULL);
    assert(create_table_struct("", users_cols, 4) == NULL);
}

static void test_insert_and_read(void)
{
    Table *t = create_table_struct("users", users_cols, 4);
    Value v[] = { val_int(7), val_int(20), val_float(13.5), val_text("Alice") };

    long id = insert_row(t, v);
    assert(id == 0);
    assert(table_row_count(t) == 1);

    const unsigned char *row = get_row(t, 0);
    assert(row != NULL);
    assert(row_get_int(t, row, 0) == 7);
    assert(row_get_int(t, row, 1) == 20);
    assert(row_get_float(t, row, 2) == 13.5);
    assert(strcmp(row_get_text(t, row, 3), "Alice") == 0);

    assert(get_row(t, 1) == NULL);
    free_table(t);
}

static void test_insert_rejected(void)
{
    Table *t = create_table_struct("users", users_cols, 4);

    Value wrong_type[] = { val_text("x"), val_int(20), val_float(1.0), val_text("a") };
    assert(insert_row(t, wrong_type) == -1);

    char long_name[64];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[63] = '\0';
    Value too_long[] = { val_int(1), val_int(20), val_float(1.0), val_text(long_name) };
    assert(insert_row(t, too_long) == -1);

    assert(table_row_count(t) == 0);
    free_table(t);
}

static void test_many_rows(void)
{
    Table *t = create_table_struct("users", users_cols, 4);
    const int n = 100000;
    char name[32];

    for (int i = 0; i < n; i++) {
        snprintf(name, sizeof(name), "user_%d", i);
        Value v[] = { val_int(i), val_int(18 + i % 50), val_float(i * 0.5), val_text(name) };
        assert(insert_row(t, v) == i);
    }
    assert(table_row_count(t) == (size_t)n);
    assert(t->rows.capacity >= (size_t)n);

    for (int i = 0; i < n; i += 997) {
        const unsigned char *row = get_row(t, (size_t)i);
        snprintf(name, sizeof(name), "user_%d", i);
        assert(row_get_int(t, row, 0) == i);
        assert(row_get_int(t, row, 1) == 18 + i % 50);
        assert(strcmp(row_get_text(t, row, 3), name) == 0);
    }
    free_table(t);
}

int main(void)
{
    test_schema_offsets();
    test_padding();
    test_invalid_schema();
    test_insert_and_read();
    test_insert_rejected();
    test_many_rows();
    printf("test_table : OK\n");
    return 0;
}
