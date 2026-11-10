#ifndef SCHEMA_H
#define SCHEMA_H

#include <stddef.h>

#define MAX_NAME_LEN 32

typedef enum {
    COL_INT,
    COL_FLOAT,
    COL_TEXT
} ColType;

typedef struct {
    const char *name;
    ColType type;
    size_t text_len;
} ColumnDef;

typedef struct {
    char name[MAX_NAME_LEN];
    ColType type;
    size_t size;
    size_t offset;
} Column;

typedef struct {
    Column *cols;
    size_t ncols;
    size_t row_size;
} Schema;

Schema *schema_create(const ColumnDef *defs, size_t ncols);
void schema_free(Schema *s);
int schema_find_column(const Schema *s, const char *name);
const char *coltype_name(ColType type);

#endif
