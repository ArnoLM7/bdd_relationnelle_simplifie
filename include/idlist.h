#ifndef IDLIST_H
#define IDLIST_H

#include <stddef.h>

typedef struct {
    size_t *ids;
    size_t count;
    size_t capacity;
} IdList;

int idlist_push(IdList *l, size_t id);
void idlist_free(IdList *l);

#endif
