#include "storage.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

int rowstore_init(RowStore *rs, size_t row_size)
{
    if (rs == NULL || row_size == 0)
        return -1;
    rs->data = NULL;
    rs->row_size = row_size;
    rs->count = 0;
    rs->capacity = 0;
    return 0;
}

void rowstore_free(RowStore *rs)
{
    if (rs == NULL)
        return;
    free(rs->data);
    rs->data = NULL;
    rs->count = 0;
    rs->capacity = 0;
}

static int rowstore_grow(RowStore *rs)
{
    size_t new_cap = rs->capacity ? rs->capacity * 2 : INITIAL_CAPACITY;
    if (new_cap > SIZE_MAX / rs->row_size)
        return -1;

    unsigned char *tmp = realloc(rs->data, new_cap * rs->row_size);
    if (tmp == NULL)
        return -1;

    rs->data = tmp;
    rs->capacity = new_cap;
    return 0;
}

unsigned char *rowstore_append(RowStore *rs)
{
    if (rs->count == rs->capacity && rowstore_grow(rs) != 0)
        return NULL;

    unsigned char *row = rs->data + rs->count * rs->row_size;
    memset(row, 0, rs->row_size);
    rs->count++;
    return row;
}

unsigned char *rowstore_get(const RowStore *rs, size_t id)
{
    if (id >= rs->count)
        return NULL;
    return rs->data + id * rs->row_size;
}
