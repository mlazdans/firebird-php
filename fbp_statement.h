#ifndef FBP_STATEMENT_H
#define FBP_STATEMENT_H

#include <ibase.h>
#include "php.h"

#include "transaction.h"

typedef enum {
    FBP_STMT_EXECUTE = 1,
    FBP_STMT_EXECUTE2,
    FBP_STMT_EXECUTE_IMMEDIATE,
} firebird_stmt_execute_fn;

typedef struct firebird_bind_buf {
    union {
        FB_BOOLEAN bval;
        short sval;
        float fval;
        ISC_LONG lval;
        ISC_QUAD qval;
        ISC_TIMESTAMP tsval;
        ISC_DATE dtval;
        ISC_TIME tmval;
    } val;
    // short sqlind;
} firebird_bind_buf;

typedef struct firebird_stmt {
    void *sptr;
    firebird_trans *tr;

    // void *in_metadata, *out_metadata;
    // unsigned char *in_buffer, *out_buffer;
    // unsigned int in_buffer_size, out_buffer_size;

    // isc_stmt_handle stmt_handle;
    // isc_db_handle *db_handle;
    // isc_tr_handle *tr_handle;
    // XSQLDA *in_sqlda, *out_sqlda;
    unsigned char statement_type, did_fake_fetch, is_cursor_open, is_exhausted;
    unsigned short in_array_cnt, out_array_cnt;
    // firebird_bind_buf *bind_buf;
    // unsigned short dialect;
    const char *sql;
    unsigned int sql_len;

    const ISC_SCHAR *name;
    ISC_ULONG insert_count, update_count, delete_count, affected_count;

    unsigned int in_vars_count, out_vars_count;

    zend_object std;
} firebird_stmt;

fbp_declare_object_accessor(firebird_stmt);

typedef struct firebird_vary {
    unsigned short vary_length;
    unsigned char vary_string[1];
} firebird_vary;

// void fbp_alloc_xsqlda(XSQLDA *sqlda);
// void fbp_free_xsqlda(XSQLDA *sqlda);

int fbp_update_statement_info(firebird_stmt *stmt);
// int fbp_statement_prepare(firebird_stmt *stmt, const ISC_SCHAR *sql);
int fbp_statement_execute(firebird_stmt *stmt, zval *bind_args, uint32_t num_bind_args, firebird_stmt_execute_fn exfn_in);
// int fbp_statement_bind(firebird_stmt *stmt, zval *b_vars);

#endif /* FBP_STATEMENT_H */
