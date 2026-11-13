#include "display.h"

#include <stdlib.h>
#include <string.h>

#define NUM_WIDTH 10
#define MAX_TEXT_WIDTH 24

static int column_width(const Column *c)
{
    int w = (c->type == COL_TEXT) ? (int)c->size - 1 : NUM_WIDTH;
    if (w > MAX_TEXT_WIDTH)
        w = MAX_TEXT_WIDTH;
    int name_len = (int)strlen(c->name);
    return name_len > w ? name_len : w;
}

static void print_separator(const Schema *s, const size_t *cols, size_t ncols, FILE *out)
{
    for (size_t i = 0; i < ncols; i++) {
        fputc('+', out);
        int w = column_width(&s->cols[cols[i]]) + 2;
        for (int k = 0; k < w; k++)
            fputc('-', out);
    }
    fputs("+\n", out);
}

static void print_cell(const Table *t, const unsigned char *row, size_t col, FILE *out)
{
    const Column *c = &t->schema->cols[col];
    int w = column_width(c);

    switch (c->type) {
    case COL_INT:
        fprintf(out, "| %*d ", w, row_get_int(t, row, col));
        break;
    case COL_FLOAT:
        fprintf(out, "| %*.2f ", w, row_get_float(t, row, col));
        break;
    case COL_TEXT:
        fprintf(out, "| %-*.*s ", w, w, row_get_text(t, row, col));
        break;
    }
}

static void print_rows(const Table *t, const size_t *ids, size_t nrows,
                       const size_t *cols, size_t ncols, FILE *out)
{
    const Schema *s = t->schema;

    print_separator(s, cols, ncols, out);
    for (size_t i = 0; i < ncols; i++)
        fprintf(out, "| %-*s ", column_width(&s->cols[cols[i]]), s->cols[cols[i]].name);
    fputs("|\n", out);
    print_separator(s, cols, ncols, out);

    for (size_t r = 0; r < nrows; r++) {
        const unsigned char *row = get_row(t, ids ? ids[r] : r);
        for (size_t i = 0; i < ncols; i++)
            print_cell(t, row, cols[i], out);
        fputs("|\n", out);
    }

    print_separator(s, cols, ncols, out);
    fprintf(out, "%zu ligne(s)\n", nrows);
}

static size_t *all_columns(const Schema *s)
{
    size_t *cols = malloc(s->ncols * sizeof(size_t));
    if (cols == NULL)
        return NULL;
    for (size_t i = 0; i < s->ncols; i++)
        cols[i] = i;
    return cols;
}

void print_schema(const Table *t, FILE *out)
{
    const Schema *s = t->schema;
    fprintf(out, "%s (", t->name);
    for (size_t i = 0; i < s->ncols; i++) {
        const Column *c = &s->cols[i];
        if (i > 0)
            fputs(", ", out);
        if (c->type == COL_TEXT)
            fprintf(out, "%s TEXT(%zu)", c->name, c->size - 1);
        else
            fprintf(out, "%s %s", c->name, coltype_name(c->type));
    }
    fprintf(out, ")  row_size=%zu\n", s->row_size);
}

void print_table(const Table *t, FILE *out)
{
    size_t *cols = all_columns(t->schema);
    if (cols == NULL)
        return;
    print_rows(t, NULL, table_row_count(t), cols, t->schema->ncols, out);
    free(cols);
}

int print_result(const Table *t, const ResultSet *rs, const char **columns, size_t ncolumns, FILE *out)
{
    if (t == NULL || rs == NULL)
        return -1;

    size_t *cols;
    size_t ncols;

    if (columns == NULL || ncolumns == 0) {
        cols = all_columns(t->schema);
        ncols = t->schema->ncols;
    } else {
        cols = malloc(ncolumns * sizeof(size_t));
        ncols = ncolumns;
        if (cols != NULL) {
            for (size_t i = 0; i < ncolumns; i++) {
                int idx = schema_find_column(t->schema, columns[i]);
                if (idx < 0) {
                    fprintf(out, "colonne inconnue : %s\n", columns[i]);
                    free(cols);
                    return -1;
                }
                cols[i] = (size_t)idx;
            }
        }
    }

    if (cols == NULL)
        return -1;

    print_rows(t, rs->row_ids, rs->count, cols, ncols, out);
    free(cols);
    return 0;
}
