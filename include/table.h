#ifndef TABLE_H
#define TABLE_H

#include <stddef.h>
#include <stdint.h>

#include "schema.h"
#include "storage.h"

typedef struct {
    ColType type;
    union {
        int32_t i;
        double f;
        const char *s;
    } as;
} Value;

typedef struct {
    char name[MAX_NAME_LEN];
    Schema *schema;
    RowStore rows;
} Table;

Value val_int(int32_t x);
Value val_float(double x);
Value val_text(const char *s);

Table *create_table_struct(const char *name, const ColumnDef *cols, size_t ncols);
void free_table(Table *t);

long insert_row(Table *t, const Value *values);
const unsigned char *get_row(const Table *t, size_t row_id);
size_t table_row_count(const Table *t);

int32_t row_get_int(const Table *t, const unsigned char *row, size_t col);
double row_get_float(const Table *t, const unsigned char *row, size_t col);
const char *row_get_text(const Table *t, const unsigned char *row, size_t col);

#endif
