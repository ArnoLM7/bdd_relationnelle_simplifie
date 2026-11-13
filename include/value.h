#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>

#include "schema.h"

typedef struct {
    ColType type;
    union {
        int32_t i;
        double f;
        const char *s;
    } as;
} Value;

Value val_int(int32_t x);
Value val_float(double x);
Value val_text(const char *s);

int value_compare(const Value *a, const Value *b);
uint64_t value_hash(const Value *v);
int value_dup(Value *dst, const Value *src);
void value_release(Value *v);

#endif
