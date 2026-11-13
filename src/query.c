#include "query.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "idlist.h"
#include "index.h"

typedef struct {
    size_t col;
    size_t offset;
    ColType type;
    Op op;
    Value v1;
    Value v2;
} BoundPred;

Predicate pred(const char *col, Op op, Value v)
{
    Predicate p;
    p.col = col;
    p.op = op;
    p.v1 = v;
    p.v2 = v;
    return p;
}

Predicate pred_between(const char *col, Value low, Value high)
{
    Predicate p;
    p.col = col;
    p.op = OP_BETWEEN;
    p.v1 = low;
    p.v2 = high;
    return p;
}

static int valid_value(const Value *v, ColType type)
{
    if (v->type != type)
        return 0;
    if (type == COL_TEXT && v->as.s == NULL)
        return 0;
    return 1;
}

static int bind_predicates(const Table *t, const Predicate *preds, size_t n, BoundPred *out)
{
    for (size_t i = 0; i < n; i++) {
        int idx = schema_find_column(t->schema, preds[i].col);
        if (idx < 0)
            return -1;

        const Column *c = &t->schema->cols[idx];
        if ((int)preds[i].op < OP_EQ || (int)preds[i].op > OP_BETWEEN)
            return -1;
        if (!valid_value(&preds[i].v1, c->type))
            return -1;
        if (preds[i].op == OP_BETWEEN && !valid_value(&preds[i].v2, c->type))
            return -1;

        out[i].col = (size_t)idx;
        out[i].offset = c->offset;
        out[i].type = c->type;
        out[i].op = preds[i].op;
        out[i].v1 = preds[i].v1;
        out[i].v2 = preds[i].v2;
    }
    return 0;
}

static int pred_cost(const BoundPred *p)
{
    return p->type == COL_TEXT ? 1 : 0;
}

static void order_predicates(BoundPred *p, size_t n)
{
    for (size_t i = 1; i < n; i++) {
        BoundPred cur = p[i];
        size_t j = i;
        while (j > 0 && pred_cost(&p[j - 1]) > pred_cost(&cur)) {
            p[j] = p[j - 1];
            j--;
        }
        p[j] = cur;
    }
}

static int compare_field(const unsigned char *row, const BoundPred *p, const Value *v)
{
    const unsigned char *field = row + p->offset;

    switch (p->type) {
    case COL_INT: {
        int32_t x;
        memcpy(&x, field, sizeof(x));
        return (x > v->as.i) - (x < v->as.i);
    }
    case COL_FLOAT: {
        double x;
        memcpy(&x, field, sizeof(x));
        return (x > v->as.f) - (x < v->as.f);
    }
    case COL_TEXT: {
        int c = strcmp((const char *)field, v->as.s);
        return (c > 0) - (c < 0);
    }
    }
    return 0;
}

static int match(const unsigned char *row, const BoundPred *p)
{
    int c = compare_field(row, p, &p->v1);

    switch (p->op) {
    case OP_EQ:
        return c == 0;
    case OP_NE:
        return c != 0;
    case OP_LT:
        return c < 0;
    case OP_LE:
        return c <= 0;
    case OP_GT:
        return c > 0;
    case OP_GE:
        return c >= 0;
    case OP_BETWEEN:
        return c >= 0 && compare_field(row, p, &p->v2) <= 0;
    }
    return 0;
}

static int match_all(const unsigned char *row, const BoundPred *preds, size_t n, size_t skip)
{
    for (size_t i = 0; i < n; i++) {
        if (i != skip && !match(row, &preds[i]))
            return 0;
    }
    return 1;
}

static int result_push(ResultSet *rs, size_t id)
{
    if (rs->count == rs->capacity) {
        size_t new_cap = rs->capacity ? rs->capacity * 2 : 16;
        size_t *tmp = realloc(rs->row_ids, new_cap * sizeof(size_t));
        if (tmp == NULL)
            return -1;
        rs->row_ids = tmp;
        rs->capacity = new_cap;
    }
    rs->row_ids[rs->count++] = id;
    return 0;
}

static int plan_score(const Table *t, const BoundPred *p, const Index **chosen)
{
    const Index *h = table_find_index(t, p->col, INDEX_HASH);
    const Index *a = table_find_index(t, p->col, INDEX_AVL);

    if (p->op == OP_EQ && h != NULL) {
        *chosen = h;
        return 4;
    }
    if (a == NULL || p->op == OP_NE)
        return 0;

    *chosen = a;
    if (p->op == OP_EQ)
        return 3;
    if (p->op == OP_BETWEEN)
        return 2;
    return 1;
}

static size_t choose_plan(const Table *t, const BoundPred *preds, size_t n, const Index **best)
{
    size_t best_i = n;
    int best_score = 0;
    *best = NULL;

    for (size_t i = 0; i < n; i++) {
        const Index *idx = NULL;
        int score = plan_score(t, &preds[i], &idx);
        if (score > best_score) {
            best_score = score;
            best_i = i;
            *best = idx;
        }
    }
    return best_i;
}

static int avl_candidates(const AvlIndex *a, const BoundPred *p, IdList *out)
{
    switch (p->op) {
    case OP_LT:
        return avl_range(a, NULL, 0, &p->v1, 0, out);
    case OP_LE:
        return avl_range(a, NULL, 0, &p->v1, 1, out);
    case OP_GT:
        return avl_range(a, &p->v1, 0, NULL, 0, out);
    case OP_GE:
        return avl_range(a, &p->v1, 1, NULL, 0, out);
    case OP_BETWEEN:
        return avl_range(a, &p->v1, 1, &p->v2, 1, out);
    case OP_EQ:
    case OP_NE:
        break;
    }
    return -1;
}

static int scan(const Table *t, const BoundPred *preds, size_t n, size_t limit, ResultSet *rs)
{
    size_t nrows = table_row_count(t);
    for (size_t id = 0; id < nrows; id++) {
        if (limit > 0 && rs->count >= limit)
            break;
        rs->examined++;
        if (match_all(get_row(t, id), preds, n, n) && result_push(rs, id) != 0)
            return -1;
    }
    return 0;
}

static int filter_candidates(const Table *t, const size_t *ids, size_t count,
                             const BoundPred *preds, size_t n, size_t skip,
                             size_t limit, ResultSet *rs)
{
    for (size_t k = 0; k < count; k++) {
        if (limit > 0 && rs->count >= limit)
            break;
        rs->examined++;
        if (match_all(get_row(t, ids[k]), preds, n, skip) && result_push(rs, ids[k]) != 0)
            return -1;
    }
    return 0;
}

static int index_lookup(const Table *t, const Index *idx, const BoundPred *preds, size_t n,
                        size_t chosen, size_t limit, ResultSet *rs)
{
    const BoundPred *p = &preds[chosen];
    const IdList *found = NULL;
    IdList range = { NULL, 0, 0 };

    if (idx->kind == INDEX_HASH) {
        found = hash_find(idx->impl.hash, &p->v1);
    } else if (p->op == OP_EQ) {
        found = avl_find(idx->impl.avl, &p->v1);
    } else {
        if (avl_candidates(idx->impl.avl, p, &range) != 0) {
            idlist_free(&range);
            return -1;
        }
        found = &range;
    }

    int ret = 0;
    if (found != NULL)
        ret = filter_candidates(t, found->ids, found->count, preds, n, chosen, limit, rs);
    idlist_free(&range);
    return ret;
}

ResultSet *select_values(const Table *t, const Predicate *preds, size_t npreds, size_t limit)
{
    if (t == NULL || (npreds > 0 && preds == NULL))
        return NULL;

    BoundPred *bound = NULL;
    if (npreds > 0) {
        bound = malloc(npreds * sizeof(*bound));
        if (bound == NULL)
            return NULL;
        if (bind_predicates(t, preds, npreds, bound) != 0) {
            free(bound);
            return NULL;
        }
        order_predicates(bound, npreds);
    }

    ResultSet *rs = calloc(1, sizeof(*rs));
    if (rs == NULL) {
        free(bound);
        return NULL;
    }

    const Index *idx = NULL;
    size_t chosen = choose_plan(t, bound, npreds, &idx);

    int ret;
    if (idx == NULL) {
        rs->plan = PLAN_SCAN;
        ret = scan(t, bound, npreds, limit, rs);
    } else {
        rs->plan = idx->kind == INDEX_HASH ? PLAN_HASH : PLAN_AVL;
        rs->plan_col = idx->col;
        ret = index_lookup(t, idx, bound, npreds, chosen, limit, rs);
    }

    free(bound);
    if (ret != 0) {
        free_result(rs);
        return NULL;
    }
    return rs;
}

void free_result(ResultSet *rs)
{
    if (rs == NULL)
        return;
    free(rs->row_ids);
    free(rs);
}

const char *plan_name(PlanKind plan)
{
    switch (plan) {
    case PLAN_SCAN:
        return "full scan";
    case PLAN_HASH:
        return "index HASH";
    case PLAN_AVL:
        return "index AVL";
    }
    return "?";
}
