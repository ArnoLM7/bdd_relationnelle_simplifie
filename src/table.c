#include "table.h"

#include <stdlib.h>
#include <string.h>

Value val_int(int32_t x)
{
    Value v;
    v.type = COL_INT;
    v.as.i = x;
    return v;
}

Value val_float(double x)
{
    Value v;
    v.type = COL_FLOAT;
    v.as.f = x;
    return v;
}

Value val_text(const char *s)
{
    Value v;
    v.type = COL_TEXT;
    v.as.s = s;
    return v;
}

Table *create_table_struct(const char *name, const ColumnDef *cols, size_t ncols)
{
    if (name == NULL || name[0] == '\0' || strlen(name) >= MAX_NAME_LEN)
        return NULL;

    Table *t = malloc(sizeof(*t));
    if (t == NULL)
        return NULL;

    t->schema = schema_create(cols, ncols);
    if (t->schema == NULL) {
        free(t);
        return NULL;
    }

    strcpy(t->name, name);
    rowstore_init(&t->rows, t->schema->row_size);
    return t;
}

void free_table(Table *t)
{
    if (t == NULL)
        return;
    rowstore_free(&t->rows);
    schema_free(t->schema);
    free(t);
}

static int check_values(const Schema *s, const Value *values)
{
    for (size_t i = 0; i < s->ncols; i++) {
        const Column *c = &s->cols[i];
        if (values[i].type != c->type)
            return 0;
        if (c->type == COL_TEXT) {
            if (values[i].as.s == NULL)
                return 0;
            if (strlen(values[i].as.s) >= c->size)
                return 0;
        }
    }
    return 1;
}

long insert_row(Table *t, const Value *values)
{
    if (t == NULL || values == NULL)
        return -1;

    const Schema *s = t->schema;
    if (!check_values(s, values))
        return -1;

    unsigned char *row = rowstore_append(&t->rows);
    if (row == NULL)
        return -1;

    for (size_t i = 0; i < s->ncols; i++) {
        const Column *c = &s->cols[i];
        unsigned char *dst = row + c->offset;
        switch (c->type) {
        case COL_INT:
            memcpy(dst, &values[i].as.i, sizeof(int32_t));
            break;
        case COL_FLOAT:
            memcpy(dst, &values[i].as.f, sizeof(double));
            break;
        case COL_TEXT:
            memcpy(dst, values[i].as.s, strlen(values[i].as.s) + 1);
            break;
        }
    }

    return (long)(t->rows.count - 1);
}

const unsigned char *get_row(const Table *t, size_t row_id)
{
    if (t == NULL)
        return NULL;
    return rowstore_get(&t->rows, row_id);
}

size_t table_row_count(const Table *t)
{
    return t ? t->rows.count : 0;
}

int32_t row_get_int(const Table *t, const unsigned char *row, size_t col)
{
    int32_t x;
    memcpy(&x, row + t->schema->cols[col].offset, sizeof(x));
    return x;
}

double row_get_float(const Table *t, const unsigned char *row, size_t col)
{
    double x;
    memcpy(&x, row + t->schema->cols[col].offset, sizeof(x));
    return x;
}

const char *row_get_text(const Table *t, const unsigned char *row, size_t col)
{
    return (const char *)(row + t->schema->cols[col].offset);
}
