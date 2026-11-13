#include "value.h"

#include <stdlib.h>
#include <string.h>

Value val_int(int32_t x)
{
    Value v;
    v.type = COL_INT;
    v.as.i = x;
    return v;
}

Value val_float(double x)
{
    Value v;
    v.type = COL_FLOAT;
    v.as.f = x;
    return v;
}

Value val_text(const char *s)
{
    Value v;
    v.type = COL_TEXT;
    v.as.s = s;
    return v;
}

int value_compare(const Value *a, const Value *b)
{
    switch (a->type) {
    case COL_INT:
        return (a->as.i > b->as.i) - (a->as.i < b->as.i);
    case COL_FLOAT:
        return (a->as.f > b->as.f) - (a->as.f < b->as.f);
    case COL_TEXT: {
        int c = strcmp(a->as.s, b->as.s);
        return (c > 0) - (c < 0);
    }
    }
    return 0;
}

static uint64_t mix64(uint64_t x)
{
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static uint64_t fnv1a(const char *s)
{
    uint64_t h = 14695981039346656037ULL;
    for (; *s; s++) {
        h ^= (unsigned char)*s;
        h *= 1099511628211ULL;
    }
    return h;
}

uint64_t value_hash(const Value *v)
{
    switch (v->type) {
    case COL_INT:
        return mix64((uint64_t)(uint32_t)v->as.i);
    case COL_FLOAT: {
        double f = v->as.f == 0.0 ? 0.0 : v->as.f;
        uint64_t bits;
        memcpy(&bits, &f, sizeof(bits));
        return mix64(bits);
    }
    case COL_TEXT:
        return fnv1a(v->as.s);
    }
    return 0;
}

int value_dup(Value *dst, const Value *src)
{
    *dst = *src;
    if (src->type != COL_TEXT)
        return 0;

    size_t len = strlen(src->as.s);
    char *copy = malloc(len + 1);
    if (copy == NULL)
        return -1;
    memcpy(copy, src->as.s, len + 1);
    dst->as.s = copy;
    return 0;
}

void value_release(Value *v)
{
    if (v->type == COL_TEXT) {
        free((char *)v->as.s);
        v->as.s = NULL;
    }
}
