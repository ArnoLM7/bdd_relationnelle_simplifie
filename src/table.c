#include "table.h"

#include <stdlib.h>
#include <string.h>

#include "index.h"

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
    t->indexes = NULL;
    t->nindexes = 0;
    return t;
}

void free_table(Table *t)
{
    if (t == NULL)
        return;
    for (size_t i = 0; i < t->nindexes; i++)
        index_free(t->indexes[i]);
    free(t->indexes);
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

    size_t id = t->rows.count - 1;
    for (size_t i = 0; i < t->nindexes; i++) {
        Index *idx = t->indexes[i];
        if (index_insert(idx, &values[idx->col], id) != 0)
            return -1;
    }

    return (long)id;
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

Value row_get_value(const Table *t, const unsigned char *row, size_t col)
{
    switch (t->schema->cols[col].type) {
    case COL_INT:
        return val_int(row_get_int(t, row, col));
    case COL_FLOAT:
        return val_float(row_get_float(t, row, col));
    case COL_TEXT:
        break;
    }
    return val_text(row_get_text(t, row, col));
}
