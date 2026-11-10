#include "schema.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static size_t type_size(ColType type, size_t text_len)
{
    switch (type) {
    case COL_INT:
        return sizeof(int32_t);
    case COL_FLOAT:
        return sizeof(double);
    case COL_TEXT:
        return text_len + 1;
    }
    return 0;
}

static size_t type_align(ColType type)
{
    switch (type) {
    case COL_INT:
        return sizeof(int32_t);
    case COL_FLOAT:
        return sizeof(double);
    case COL_TEXT:
        return 1;
    }
    return 1;
}

static size_t align_up(size_t x, size_t a)
{
    return (x + a - 1) / a * a;
}

static int valid_def(const ColumnDef *d)
{
    if (d->name == NULL || d->name[0] == '\0')
        return 0;
    if (strlen(d->name) >= MAX_NAME_LEN)
        return 0;
    if (d->type == COL_TEXT && d->text_len == 0)
        return 0;
    return 1;
}

Schema *schema_create(const ColumnDef *defs, size_t ncols)
{
    if (defs == NULL || ncols == 0)
        return NULL;

    Schema *s = malloc(sizeof(*s));
    if (s == NULL)
        return NULL;

    s->cols = calloc(ncols, sizeof(Column));
    if (s->cols == NULL) {
        free(s);
        return NULL;
    }
    s->ncols = ncols;

    size_t offset = 0;
    size_t max_align = 1;

    for (size_t i = 0; i < ncols; i++) {
        if (!valid_def(&defs[i])) {
            schema_free(s);
            return NULL;
        }
        for (size_t j = 0; j < i; j++) {
            if (strcmp(defs[i].name, defs[j].name) == 0) {
                schema_free(s);
                return NULL;
            }
        }

        Column *c = &s->cols[i];
        strcpy(c->name, defs[i].name);
        c->type = defs[i].type;
        c->size = type_size(c->type, defs[i].text_len);

        size_t a = type_align(c->type);
        if (a > max_align)
            max_align = a;

        offset = align_up(offset, a);
        c->offset = offset;
        offset += c->size;
    }

    s->row_size = align_up(offset, max_align);
    return s;
}

void schema_free(Schema *s)
{
    if (s == NULL)
        return;
    free(s->cols);
    free(s);
}

int schema_find_column(const Schema *s, const char *name)
{
    if (s == NULL || name == NULL)
        return -1;
    for (size_t i = 0; i < s->ncols; i++) {
        if (strcmp(s->cols[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}

const char *coltype_name(ColType type)
{
    switch (type) {
    case COL_INT:
        return "INT";
    case COL_FLOAT:
        return "FLOAT";
    case COL_TEXT:
        return "TEXT";
    }
    return "?";
}
