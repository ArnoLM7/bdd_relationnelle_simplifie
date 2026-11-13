#ifndef QUERY_H
#define QUERY_H

#include <stddef.h>

#include "table.h"

typedef enum {
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_BETWEEN
} Op;

typedef struct {
    const char *col;
    Op op;
    Value v1;
    Value v2;
} Predicate;

typedef struct {
    size_t *row_ids;
    size_t count;
    size_t capacity;
} ResultSet;

Predicate pred(const char *col, Op op, Value v);
Predicate pred_between(const char *col, Value low, Value high);

ResultSet *select_values(const Table *t, const Predicate *preds, size_t npreds, size_t limit);
void free_result(ResultSet *rs);

#endif
