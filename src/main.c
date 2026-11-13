#include <stdio.h>

#include "display.h"
#include "index.h"
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
    if (rs->plan == PLAN_SCAN)
        printf("plan : %s, lignes examinees : %zu\n", plan_name(rs->plan), rs->examined);
    else
        printf("plan : %s sur %s, lignes examinees : %zu\n", plan_name(rs->plan),
               t->schema->cols[rs->plan_col].name, rs->examined);
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

    Predicate q1[] = { pred("name", OP_EQ, val_text("Emma")) };
    run_query(users, "name = 'Emma' (sans index)", q1, 1, NULL, 0, 0);

    if (create_index(users, "name", INDEX_HASH) != 0 ||
        create_index(users, "age", INDEX_AVL) != 0) {
        fprintf(stderr, "creation des index impossible\n");
        free_table(users);
        return 1;
    }
    printf("\nindex crees : HASH(name), AVL(age)\n");

    run_query(users, "name = 'Emma' (avec index)", q1, 1, NULL, 0, 0);

    Predicate q2[] = { pred("age", OP_GT, val_int(21)) };
    run_query(users, "age > 21", q2, 1, NULL, 0, 0);

    Predicate q3[] = {
        pred_between("age", val_int(19), val_int(21)),
        pred("score", OP_GE, val_float(12.0)),
    };
    const char *proj3[] = { "name", "age", "score" };
    run_query(users, "name, age, score WHERE age BETWEEN 19 AND 21 AND score >= 12", q3, 2, proj3, 3, 0);

    Value late[] = { val_int(8), val_int(20), val_float(17.0), val_text("Hugo") };
    insert_row(users, late);
    Predicate q4[] = { pred("age", OP_EQ, val_int(20)) };
    run_query(users, "age = 20 (apres insertion de Hugo)", q4, 1, NULL, 0, 0);

    Predicate q5[] = { pred("score", OP_LT, val_float(12.0)) };
    run_query(users, "score < 12 (pas d'index sur score)", q5, 1, NULL, 0, 0);

    free_table(users);
    return 0;
}
