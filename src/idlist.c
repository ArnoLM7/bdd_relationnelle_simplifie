#include "idlist.h"

#include <stdlib.h>

int idlist_push(IdList *l, size_t id)
{
    if (l->count == l->capacity) {
        size_t new_cap = l->capacity ? l->capacity * 2 : 4;
        size_t *tmp = realloc(l->ids, new_cap * sizeof(size_t));
        if (tmp == NULL)
            return -1;
        l->ids = tmp;
        l->capacity = new_cap;
    }
    l->ids[l->count++] = id;
    return 0;
}

void idlist_free(IdList *l)
{
    free(l->ids);
    l->ids = NULL;
    l->count = 0;
    l->capacity = 0;
}
