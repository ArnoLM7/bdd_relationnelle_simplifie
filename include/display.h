#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>

#include "table.h"

void print_schema(const Table *t, FILE *out);
void print_table(const Table *t, FILE *out);

#endif
