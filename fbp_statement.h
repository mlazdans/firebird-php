#ifndef FBP_STATEMENT_H
#define FBP_STATEMENT_H

#include <ibase.h>
#include "php.h"

#include "fbp_transaction.h"

enum firebird_stmt_execute_fn {
    FBP_STMT_EXECUTE = 1,
    FBP_STMT_EXECUTE2,
    FBP_STMT_EXECUTE_IMMEDIATE,
};

typedef struct firebird_bind_buf {
    union {
#ifdef SQL_BOOLEAN
        FB_BOOLEAN bval;
#endif
        short sval;
        float fval;
        ISC_LONG lval;
        ISC_QUAD qval;
        ISC_TIMESTAMP tsval;
        ISC_DATE dtval;
        ISC_TIME tmval;
    } val;
    short sqlind;
} firebird_bind_buf;

typedef struct firebird_stmt {
    isc_stmt_handle stmt_handle;
    isc_db_handle *db_handle;
    isc_tr_handle *tr_handle;
    XSQLDA *in_sqlda, *out_sqlda;
    unsigned char statement_type, has_more_rows;
    unsigned short in_array_cnt, out_array_cnt;
    firebird_bind_buf *bind_buf;
    // unsigned short dialect;
    const ISC_SCHAR *query;
    const ISC_SCHAR *name;
    ISC_ULONG insert_count, update_count, delete_count, affected_count;
    zend_object std;
} firebird_stmt;

fbp_declare_object_accessor(firebird_stmt);

typedef struct firebird_vary {
    unsigned short vary_length;
    char vary_string[1];
} firebird_vary;

void fbp_alloc_xsqlda(XSQLDA *sqlda);
void fbp_free_xsqlda(XSQLDA *sqlda);

void fbp_statement_ctor(firebird_stmt *stmt, firebird_trans *tr);
void fbp_statement_free(firebird_stmt *s);
int fbp_update_statement_info(firebird_stmt *stmt);
int fbp_statement_prepare(firebird_stmt *stmt, const ISC_SCHAR *sql);
int fbp_statement_execute(firebird_stmt *stmt, zval *bind_args, uint32_t num_bind_args, enum firebird_stmt_execute_fn exfn_in);
int fbp_statement_bind(firebird_stmt *stmt, XSQLDA *sqlda, zval *b_vars);

#endif /* FBP_STATEMENT_H */
