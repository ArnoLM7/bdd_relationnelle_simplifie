#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>

#include "query.h"
#include "table.h"

void print_schema(const Table *t, FILE *out);
void print_table(const Table *t, FILE *out);
int print_result(const Table *t, const ResultSet *rs, const char **columns, size_t ncolumns, FILE *out);

#endif
