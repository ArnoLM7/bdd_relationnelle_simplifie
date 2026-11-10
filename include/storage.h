#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

typedef struct {
    unsigned char *data;
    size_t row_size;
    size_t count;
    size_t capacity;
} RowStore;

int rowstore_init(RowStore *rs, size_t row_size);
void rowstore_free(RowStore *rs);
unsigned char *rowstore_append(RowStore *rs);
unsigned char *rowstore_get(const RowStore *rs, size_t id);

#endif
