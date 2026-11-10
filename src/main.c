#include <stdio.h>

#include "display.h"
#include "table.h"

int main(void)
{
    ColumnDef cols[] = {
        { "id",    COL_INT,   0 },
        { "age",   COL_INT,   0 },
        { "score", COL_FLOAT, 0 },
        { "name",  COL_TEXT,  31 },
    };

    Table *users = create_table_struct("users", cols, 4);
    if (users == NULL) {
        fprintf(stderr, "impossible de creer la table\n");
        return 1;
    }

    Value rows[][4] = {
        { val_int(1), val_int(21), val_float(14.5), val_text("Alice") },
        { val_int(2), val_int(19), val_float(12.0), val_text("Bob") },
        { val_int(3), val_int(23), val_float(16.25), val_text("Chloe") },
        { val_int(4), val_int(20), val_float(9.75), val_text("David") },
        { val_int(5), val_int(22), val_float(18.0), val_text("Emma") },
    };

    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        if (insert_row(users, rows[i]) < 0)
            fprintf(stderr, "insertion %zu refusee\n", i);
    }

    print_schema(users, stdout);
    print_table(users, stdout);

    free_table(users);
    return 0;
}
