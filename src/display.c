#include "display.h"

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

static void print_separator(const Schema *s, FILE *out)
{
    for (size_t i = 0; i < s->ncols; i++) {
        fputc('+', out);
        int w = column_width(&s->cols[i]) + 2;
        for (int k = 0; k < w; k++)
            fputc('-', out);
    }
    fputs("+\n", out);
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
    const Schema *s = t->schema;

    print_separator(s, out);
    for (size_t i = 0; i < s->ncols; i++)
        fprintf(out, "| %-*s ", column_width(&s->cols[i]), s->cols[i].name);
    fputs("|\n", out);
    print_separator(s, out);

    for (size_t r = 0; r < table_row_count(t); r++) {
        const unsigned char *row = get_row(t, r);
        for (size_t i = 0; i < s->ncols; i++) {
            int w = column_width(&s->cols[i]);
            switch (s->cols[i].type) {
            case COL_INT:
                fprintf(out, "| %*d ", w, row_get_int(t, row, i));
                break;
            case COL_FLOAT:
                fprintf(out, "| %*.2f ", w, row_get_float(t, row, i));
                break;
            case COL_TEXT:
                fprintf(out, "| %-*.*s ", w, w, row_get_text(t, row, i));
                break;
            }
        }
        fputs("|\n", out);
    }

    print_separator(s, out);
    fprintf(out, "%zu ligne(s)\n", table_row_count(t));
}
