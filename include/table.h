#ifndef TABLE_H
#define TABLE_H

#include <stddef.h>
#include <stdint.h>

#include "schema.h"
#include "storage.h"
#include "value.h"

struct Index;

typedef struct {
    char name[MAX_NAME_LEN];
    Schema *schema;
    RowStore rows;
    struct Index **indexes;
    size_t nindexes;
} Table;

Table *create_table_struct(const char *name, const ColumnDef *cols, size_t ncols);
void free_table(Table *t);

long insert_row(Table *t, const Value *values);
const unsigned char *get_row(const Table *t, size_t row_id);
size_t table_row_count(const Table *t);

int32_t row_get_int(const Table *t, const unsigned char *row, size_t col);
double row_get_float(const Table *t, const unsigned char *row, size_t col);
const char *row_get_text(const Table *t, const unsigned char *row, size_t col);
Value row_get_value(const Table *t, const unsigned char *row, size_t col);

#endif
