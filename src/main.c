#include <stdio.h>

#include "display.h"
#include "query.h"
#include "table.h"

static void run_query(const Table *t, const char *title, const Predicate *preds, size_t npreds,
                      const char **cols, size_t ncols, size_t limit)
{
    printf("\n%s\n", title);
    ResultSet *rs = select_values(t, preds, npreds, limit);
    if (rs == NULL) {
        printf("requete invalide\n");
        return;
    }
    print_result(t, rs, cols, ncols, stdout);
    free_result(rs);
}

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
        { val_int(1), val_int(21), val_float(14.5),  val_text("Alice") },
        { val_int(2), val_int(19), val_float(12.0),  val_text("Bob") },
        { val_int(3), val_int(23), val_float(16.25), val_text("Chloe") },
        { val_int(4), val_int(20), val_float(9.75),  val_text("David") },
        { val_int(5), val_int(22), val_float(18.0),  val_text("Emma") },
        { val_int(6), val_int(20), val_float(11.5),  val_text("Farid") },
        { val_int(7), val_int(24), val_float(15.0),  val_text("Gina") },
    };

    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        if (insert_row(users, rows[i]) < 0)
            fprintf(stderr, "insertion %zu refusee\n", i);
    }

    print_schema(users, stdout);
    print_table(users, stdout);

    Predicate q1[] = { pred("age", OP_GT, val_int(21)) };
    run_query(users, "age > 21", q1, 1, NULL, 0, 0);

    Predicate q2[] = {
        pred_between("age", val_int(19), val_int(21)),
        pred("score", OP_GE, val_float(12.0)),
    };
    const char *proj2[] = { "name", "age", "score" };
    run_query(users, "name, age, score WHERE age BETWEEN 19 AND 21 AND score >= 12", q2, 2, proj2, 3, 0);

    Predicate q3[] = { pred("name", OP_EQ, val_text("Emma")) };
    run_query(users, "name = 'Emma'", q3, 1, NULL, 0, 0);

    const char *proj4[] = { "id", "name" };
    run_query(users, "id, name LIMIT 3", NULL, 0, proj4, 2, 3);

    Predicate q5[] = { pred("ville", OP_EQ, val_text("Paris")) };
    run_query(users, "ville = 'Paris'", q5, 1, NULL, 0, 0);

    free_table(users);
    return 0;
}
