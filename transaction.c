#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Transaction_ce;
static zend_object_handlers FireBird_Transaction_object_handlers;

// static int alloc_array(firebird_array **ib_arrayp, unsigned short *array_cnt, XSQLDA *sqlda, zend_object *obj);
static void _php_firebird_alloc_xsqlda(XSQLDA *sqlda);

#define ROLLBACK    0
#define COMMIT      1
#define RETAIN      2

PHP_METHOD(Transaction, __construct) {
    zend_long trans_args = 0, lock_timeout = 0;
    zval *connection;
    bool trans_args_is_null = 1, lock_timeout_is_null = 1;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_OBJECT_OF_CLASS(connection, FireBird_Connection_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(trans_args, trans_args_is_null)
        Z_PARAM_LONG_OR_NULL(lock_timeout, lock_timeout_is_null)
    ZEND_PARSE_PARAMETERS_END();

    zend_update_property(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "connection", sizeof("connection") - 1, connection);

    if(!trans_args_is_null) zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "trans_args", sizeof("trans_args") - 1, trans_args);
    if(!lock_timeout_is_null) zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "lock_timeout", sizeof("lock_timeout") - 1, lock_timeout);

    firebird_connection *conn = Z_CONNECTION_P(connection);
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);
    tr->db_handle = conn->handle;
}

PHP_METHOD(Transaction, start) {
    ZEND_PARSE_PARAMETERS_NONE();

    zval rv, *val;
    zend_long trans_args = 0, lock_timeout = 0;
    ISC_STATUS_ARRAY status;
    ISC_STATUS result;
    char tpb[TPB_MAX_SIZE];
    unsigned short tpb_len = 0;
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);

    val = zend_read_property(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "trans_args", sizeof("trans_args") - 1, 1, &rv);
    trans_args = Z_LVAL_P(val);

    val = zend_read_property(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "lock_timeout", sizeof("lock_timeout") - 1, 1, &rv);
    lock_timeout = Z_LVAL_P(val);

    php_printf("trans_args=%d, lock_timeout=%d\n", trans_args, lock_timeout);

    populate_trans(trans_args, lock_timeout, tpb, &tpb_len);
    if(isc_start_transaction(status, &tr->tr_handle, 1, &tr->db_handle, tpb_len, tpb)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

bool process_trans(zend_object *obj, int commit) {
    zval rv, *val;
    ISC_STATUS result;
    ISC_STATUS_ARRAY status;
    firebird_trans *tr = Z_TRANSACTION_O(obj);

    switch (commit) {
        default: /* == case ROLLBACK: */
            result = isc_rollback_transaction(status, &tr->tr_handle);
            break;
        case COMMIT:
            result = isc_commit_transaction(status, &tr->tr_handle);
            break;
        case (ROLLBACK | RETAIN):
            result = isc_rollback_retaining(status, &tr->tr_handle);
            break;
        case (COMMIT | RETAIN):
            result = isc_commit_retaining(status, &tr->tr_handle);
            break;
    }

    if (result) {
        update_err_props(status, FireBird_Transaction_ce, obj);
        return false;
    } else {
        return true;
    }
}

PHP_METHOD(Transaction, commit) {
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_BOOL(process_trans(Z_OBJ_P(ZEND_THIS), COMMIT));
}

PHP_METHOD(Transaction, commit_ret) {
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_BOOL(process_trans(Z_OBJ_P(ZEND_THIS), COMMIT | RETAIN));
}

PHP_METHOD(Transaction, rollback) {
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_BOOL(process_trans(Z_OBJ_P(ZEND_THIS), ROLLBACK));
}

PHP_METHOD(Transaction, rollback_ret) {
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_BOOL(process_trans(Z_OBJ_P(ZEND_THIS), ROLLBACK | RETAIN));
}

PHP_METHOD(Transaction, query)
{
    zval rv, *val;
    zend_long trans_args = 0, lock_timeout = 0;
    ISC_STATUS_ARRAY status;
    char tpb[TPB_MAX_SIZE];
    unsigned short tpb_len = 0;
    zend_string *sql = NULL;

    // val = zend_read_property(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "transaction", sizeof("transaction") - 1, 0, &rv);
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);

    zval *bind_args = NULL;
    uint32_t num_bind_args, i;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(sql)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    php_printf("sql=%s\n", ZSTR_VAL(sql));
    for (i = 0; i < num_bind_args; i++) {
        php_printf("    arg=%s\n", zend_zval_type_name(&bind_args[i]));
    }

    object_init_ex(&rv, FireBird_Statement_ce);
    zend_object *stmt_o = Z_OBJ(rv);
    firebird_stmt *stmt = Z_STMT_O(stmt_o);

    stmt->db_handle = tr->db_handle;
    stmt->tr_handle = tr->tr_handle;
    stmt->has_more_rows = 1;

    if (isc_dsql_allocate_statement(status, &stmt->db_handle, &stmt->stmt_handle)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&rv);
        RETURN_FALSE;
    }

    stmt->out_sqlda = (XSQLDA *) emalloc(XSQLDA_LENGTH(1));
    stmt->out_sqlda->sqln = 1;
    stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;

    if (isc_dsql_prepare(status, &stmt->tr_handle, &stmt->stmt_handle, 0, ZSTR_VAL(sql), SQL_DIALECT_CURRENT, stmt->out_sqlda)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&rv);
        RETURN_FALSE;
    }

    static char info_type[] = { isc_info_sql_stmt_type };
    char result[8];

    /* find out what kind of statement was prepared */
    if (isc_dsql_sql_info(status, &stmt->stmt_handle, sizeof(info_type), info_type, sizeof(result), result)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&rv);
        RETURN_FALSE;
    }

    stmt->statement_type = result[3];

    if (stmt->out_sqlda->sqld > stmt->out_sqlda->sqln) {
        stmt->out_sqlda = erealloc(stmt->out_sqlda, XSQLDA_LENGTH(stmt->out_sqlda->sqld));
        stmt->out_sqlda->sqln = stmt->out_sqlda->sqld;
        stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;
        if (isc_dsql_describe(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->out_sqlda)) {
            update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
            zval_ptr_dtor(&rv);
            RETURN_FALSE;
        }
    }

    _php_firebird_alloc_xsqlda(stmt->out_sqlda);

    /* maybe have input placeholders? */
    stmt->in_sqlda = emalloc(XSQLDA_LENGTH(1));
    stmt->in_sqlda->sqln = 1;
    stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;
    if (isc_dsql_describe_bind(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&rv);
        RETURN_FALSE;
    }

    /* not enough input variables ? */
    if (stmt->in_sqlda->sqln < stmt->in_sqlda->sqld) {
        stmt->in_sqlda = erealloc(stmt->in_sqlda, XSQLDA_LENGTH(stmt->in_sqlda->sqld));
        stmt->in_sqlda->sqln = stmt->in_sqlda->sqld;
        stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;

        if (isc_dsql_describe_bind(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
            update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
            zval_ptr_dtor(&rv);
            RETURN_FALSE;
        }
    }

    ISC_STATUS isc_result;
    if (stmt->statement_type == isc_info_sql_stmt_exec_procedure) {
        assert(false && "TODO: isc_info_sql_stmt_exec_procedure");
        // isc_result = isc_dsql_execute2(IB_STATUS, &stmt->transtmt->handle,
        //     &stmt->stmt, SQLDA_CURRENT_VERSION, in_sqlda, out_sqlda);
    } else {
        // isc_result = isc_dsql_execute(status, &stmt->tr_handle, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda);
        isc_result = isc_dsql_execute(status, &stmt->tr_handle, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda);
    }

    if (isc_result) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&rv);
        RETURN_FALSE;
    }

    RETVAL_OBJ(stmt_o);

    // TODO: handle SQL_ARRAY

    /* no, haven't placeholders at all */
    // if (q->in_sqlda->sqld == 0) {
    //     efree(q->in_sqlda);
    //     q->in_sqlda = NULL;
    // // } else if (FAILURE == alloc_arraqy(&q->in_array, &q->in_array_cnt, q->in_sqlda, Z_OBJ_P(ZEND_THIS))) {
    // //     // update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
    // //     goto query_error;
    // }

    // if (q->out_sqlda->sqld == 0) {
    //     efree(q->out_sqlda);
    //     q->out_sqlda = NULL;
    // // } else if (FAILURE == alloc_array(&q->out_array, &q->out_array_cnt, q->out_sqlda, Z_OBJ_P(ZEND_THIS))) {
    // //     // update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
    // //     goto query_error;
    // }
}

const zend_function_entry FireBird_Transaction_methods[] = {
    PHP_ME(Transaction, __construct, arginfo_FireBird_Transaction_construct, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, start, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, commit, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, commit_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, rollback, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, rollback_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, query, arginfo_FireBird_Transaction_query, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Transaction_create(zend_class_entry *ce)
{
    firebird_trans *tr = zend_object_alloc(sizeof(firebird_trans), ce);

    zend_object_std_init(&tr->std, ce);
    object_properties_init(&tr->std, ce);

    return &tr->std;
}

static void FireBird_Transaction_free_obj(zend_object *obj)
{
    php_printf("FireBird_Transaction_free_obj\n");
    firebird_trans *tr = Z_TRANSACTION_O(obj);

    if(tr->tr_handle) {
        ISC_STATUS_ARRAY status;
        if(isc_rollback_transaction(status, &tr->tr_handle)) {
            // TODO: report errors?
        }
        tr->tr_handle = 0;
    }

    zend_object_std_dtor(&tr->std);
}

void register_FireBird_Transaction_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Transaction", FireBird_Transaction_methods);
    FireBird_Transaction_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Transaction_ce, connection, FireBird\\Connection, ZEND_ACC_PROTECTED);
    DECLARE_PROP_INT(FireBird_Transaction_ce, trans_args, ZEND_ACC_PROTECTED);
    DECLARE_PROP_INT(FireBird_Transaction_ce, lock_timeout, ZEND_ACC_PROTECTED);
    ADD_ERR_PROPS(FireBird_Transaction_ce);

    FireBird_Transaction_ce->create_object = FireBird_Transaction_create;
    FireBird_Transaction_ce->default_object_handlers = &FireBird_Transaction_object_handlers;

    memcpy(&FireBird_Transaction_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Transaction_object_handlers.offset = XtOffsetOf(firebird_trans, std);
    FireBird_Transaction_object_handlers.free_obj = FireBird_Transaction_free_obj;
}

void populate_trans(zend_long trans_argl, zend_long trans_timeout, char *last_tpb, unsigned short *len)
{
    unsigned short tpb_len = 0;

    *len = tpb_len;

    if (trans_argl == 0) {
        return;
    }

    last_tpb[tpb_len++] = isc_tpb_version3;

    /* access mode */
    if (PHP_FIREBIRD_READ == (trans_argl & PHP_FIREBIRD_READ)) {
        last_tpb[tpb_len++] = isc_tpb_read;
    } else if (PHP_FIREBIRD_WRITE == (trans_argl & PHP_FIREBIRD_WRITE)) {
        last_tpb[tpb_len++] = isc_tpb_write;
    }

    /* isolation level */
    if (PHP_FIREBIRD_COMMITTED == (trans_argl & PHP_FIREBIRD_COMMITTED)) {
        last_tpb[tpb_len++] = isc_tpb_read_committed;
        if (PHP_FIREBIRD_REC_VERSION == (trans_argl & PHP_FIREBIRD_REC_VERSION)) {
            last_tpb[tpb_len++] = isc_tpb_rec_version;
        } else if (PHP_FIREBIRD_REC_NO_VERSION == (trans_argl & PHP_FIREBIRD_REC_NO_VERSION)) {
            last_tpb[tpb_len++] = isc_tpb_no_rec_version;
        }
    } else if (PHP_FIREBIRD_CONSISTENCY == (trans_argl & PHP_FIREBIRD_CONSISTENCY)) {
        last_tpb[tpb_len++] = isc_tpb_consistency;
    } else if (PHP_FIREBIRD_CONCURRENCY == (trans_argl & PHP_FIREBIRD_CONCURRENCY)) {
        last_tpb[tpb_len++] = isc_tpb_concurrency;
    }

    /* lock resolution */
    if (PHP_FIREBIRD_NOWAIT == (trans_argl & PHP_FIREBIRD_NOWAIT)) {
        last_tpb[tpb_len++] = isc_tpb_nowait;
    } else if (PHP_FIREBIRD_WAIT == (trans_argl & PHP_FIREBIRD_WAIT)) {
        last_tpb[tpb_len++] = isc_tpb_wait;
        if (PHP_FIREBIRD_LOCK_TIMEOUT == (trans_argl & PHP_FIREBIRD_LOCK_TIMEOUT)) {
            last_tpb[tpb_len++] = isc_tpb_lock_timeout;
            last_tpb[tpb_len++] = sizeof(ISC_SHORT);
            last_tpb[tpb_len] = (ISC_SHORT)trans_timeout;
            tpb_len += sizeof(ISC_SHORT);
        }
    }

    *len = tpb_len;
}

// static int alloc_array(firebird_array **ib_arrayp, unsigned short *array_cnt, XSQLDA *sqlda, zend_object *obj)
// {
//     unsigned short i, n;
//     firebird_array *ar;
//     ISC_STATUS_ARRAY status;
//     firebird_query *q = Z_QUERY_O(obj);

//     /* first check if we have any arrays at all */
//     for (i = *array_cnt = 0; i < sqlda->sqld; ++i) {
//         if ((sqlda->sqlvar[i].sqltype & ~1) == SQL_ARRAY) {
//             ++*array_cnt;
//         }
//     }
//     if (! *array_cnt) return SUCCESS;

//     ar = safe_emalloc(sizeof(firebird_array), *array_cnt, 0);

//     for (i = n = 0; i < sqlda->sqld; ++i) {
//         unsigned short dim;
//         zend_ulong ar_size = 1;
//         XSQLVAR *var = &sqlda->sqlvar[i];

//         if ((var->sqltype & ~1) == SQL_ARRAY) {
//             firebird_array *a = &ar[n++];
//             ISC_ARRAY_DESC *ar_desc = &a->ar_desc;

//             if (isc_array_lookup_bounds(status, &q->db_handle, &q->tr_handle, var->relname, var->sqlname, ar_desc)) {
//                 update_err_props(status, FireBird_Transaction_ce, obj);
//                 efree(ar);
//                 return FAILURE;
//             }

//             switch (ar_desc->array_desc_dtype) {
//                 case blr_text:
//                 case blr_text2:
//                     a->el_type = SQL_TEXT;
//                     a->el_size = ar_desc->array_desc_length;
//                     break;
// #ifdef SQL_BOOLEAN
//                 case blr_bool:
//                     a->el_type = SQL_BOOLEAN;
//                     a->el_size = sizeof(FB_BOOLEAN);
//                     break;
// #endif
//                 case blr_short:
//                     a->el_type = SQL_SHORT;
//                     a->el_size = sizeof(short);
//                     break;
//                 case blr_long:
//                     a->el_type = SQL_LONG;
//                     a->el_size = sizeof(ISC_LONG);
//                     break;
//                 case blr_float:
//                     a->el_type = SQL_FLOAT;
//                     a->el_size = sizeof(float);
//                     break;
//                 case blr_double:
//                     a->el_type = SQL_DOUBLE;
//                     a->el_size = sizeof(double);
//                     break;
//                 case blr_int64:
//                     a->el_type = SQL_INT64;
//                     a->el_size = sizeof(ISC_INT64);
//                     break;
//                 case blr_timestamp:
//                     a->el_type = SQL_TIMESTAMP;
//                     a->el_size = sizeof(ISC_TIMESTAMP);
//                     break;
//                 case blr_sql_date:
//                     a->el_type = SQL_TYPE_DATE;
//                     a->el_size = sizeof(ISC_DATE);
//                     break;
//                 case blr_sql_time:
//                     a->el_type = SQL_TYPE_TIME;
//                     a->el_size = sizeof(ISC_TIME);
//                     break;
// #if FB_API_VER >= 40
//                 // These are converted to VARCHAR via isc_dpb_set_bind tag at connect
//                 // blr_dec64
//                 // blr_dec128
//                 // blr_int128
//                 case blr_sql_time_tz:
//                     a->el_type = SQL_TIME_TZ;
//                     a->el_size = sizeof(ISC_TIME_TZ);
//                     break;
//                 case blr_timestamp_tz:
//                     a->el_type = SQL_TIMESTAMP_TZ;
//                     a->el_size = sizeof(ISC_TIMESTAMP_TZ);
//                     break;
// #endif
//                 case blr_varying:
//                 case blr_varying2:
//                     /**
//                      * IB has a strange way of handling VARCHAR arrays. It doesn't store
//                      * the length in the first short, as with VARCHAR fields. It does,
//                      * however, expect the extra short to be allocated for each element.
//                      */
//                     a->el_type = SQL_TEXT;
//                     a->el_size = ar_desc->array_desc_length + sizeof(short);
//                     break;
//                 case blr_quad:
//                 case blr_blob_id:
//                 case blr_cstring:
//                 case blr_cstring2:
//                     /**
//                      * These types are mentioned as array types in the manual, but I
//                      * wouldn't know how to create an array field with any of these
//                      * types. I assume these types are not applicable to arrays, and
//                      * were mentioned erroneously.
//                      */
//                 default:
//                     _php_firebird_module_error("Unsupported array type %d in relation '%s' column '%s'",
//                         ar_desc->array_desc_dtype, var->relname, var->sqlname);
//                     efree(ar);
//                     return FAILURE;
//             } /* switch array_desc_type */

//             /* calculate elements count */
//             for (dim = 0; dim < ar_desc->array_desc_dimensions; dim++) {
//                 ar_size *= 1 + ar_desc->array_desc_bounds[dim].array_bound_upper
//                     -ar_desc->array_desc_bounds[dim].array_bound_lower;
//             }
//             a->ar_size = a->el_size * ar_size;
//         } /* if SQL_ARRAY */
//     } /* for column */
//     *ib_arrayp = ar;
//     return SUCCESS;
// }

// static int _php_firebird_exec(INTERNAL_FUNCTION_PARAMETERS, ibase_result **ib_resultp,
//     ibase_query *ib_query, zval *args)
// {
//     XSQLDA *in_sqlda = NULL, *out_sqlda = NULL;
//     BIND_BUF *bind_buf = NULL;
//     int i, rv = FAILURE;
//     static char info_count[] = { isc_info_sql_records };
//     char result[64];
//     ISC_STATUS isc_result;
//     int argc = ib_query->in_sqlda ? ib_query->in_sqlda->sqld : 0;

//     RESET_ERRMSG;

//     for (i = 0; i < argc; ++i) {
//         SEPARATE_ZVAL(&args[i]);
//     }

//     switch (ib_query->statement_type) {
//         isc_tr_handle tr;
//         ibase_tr_list **l;
//         ibase_trans *trans;

//         case isc_info_sql_stmt_start_trans:

//             /* a SET TRANSACTION statement should be executed with a NULL trans handle */
//             tr = 0;

//             if (isc_dsql_execute_immediate(IB_STATUS, &ib_query->link->handle, &tr, 0,
//                     ib_query->query, ib_query->dialect, NULL)) {
//                 _php_firebird_error();
//                 goto _php_firebird_exec_error;
//             }

//             trans = (ibase_trans *) emalloc(sizeof(ibase_trans));
//             trans->handle = tr;
//             trans->link_cnt = 1;
//             trans->affected_rows = 0;
//             trans->db_link[0] = ib_query->link;

//             if (ib_query->link->tr_list == NULL) {
//                 ib_query->link->tr_list = (ibase_tr_list *) emalloc(sizeof(ibase_tr_list));
//                 ib_query->link->tr_list->trans = NULL;
//                 ib_query->link->tr_list->next = NULL;
//             }

//             /* link the transaction into the connection-transaction list */
//             for (l = &ib_query->link->tr_list; *l != NULL; l = &(*l)->next);
//             *l = (ibase_tr_list *) emalloc(sizeof(ibase_tr_list));
//             (*l)->trans = trans;
//             (*l)->next = NULL;

//             RETVAL_RES(zend_register_resource(trans, le_trans));
//             Z_TRY_ADDREF_P(return_value);

//             return SUCCESS;

//         case isc_info_sql_stmt_commit:
//         case isc_info_sql_stmt_rollback:

//             if (isc_dsql_execute_immediate(IB_STATUS, &ib_query->link->handle,
//                     &ib_query->trans->handle, 0, ib_query->query, ib_query->dialect, NULL)) {
//                 _php_firebird_error();
//                 goto _php_firebird_exec_error;
//             }

//             if (ib_query->trans->handle == 0 && ib_query->trans_res != NULL) {
//                 /* transaction was released by the query and was a registered resource,
//                    so we have to release it */
//                 zend_list_delete(ib_query->trans_res);
//                 ib_query->trans_res = NULL;
//             }

//             RETVAL_TRUE;

//             return SUCCESS;

//         default:
//             RETVAL_FALSE;
//     }

//     /* allocate sqlda and output buffers */
//     if (ib_query->out_sqlda) { /* output variables in select, select for update */
//         ibase_result *res;

//         IBDEBUG("Query wants XSQLDA for output");
//         res = emalloc(sizeof(ibase_result)+sizeof(ibase_array)*max(0,ib_query->out_array_cnt-1));
//         res->link = ib_query->link;
//         res->trans = ib_query->trans;
//         res->stmt = ib_query->stmt;
//         GC_ADDREF(res->stmt_res = ib_query->stmt_res);

//         res->statement_type = ib_query->statement_type;
//         res->has_more_rows = 1;

//         out_sqlda = res->out_sqlda = emalloc(XSQLDA_LENGTH(ib_query->out_sqlda->sqld));
//         memcpy(out_sqlda, ib_query->out_sqlda, XSQLDA_LENGTH(ib_query->out_sqlda->sqld));
//         _php_firebird_alloc_xsqlda(out_sqlda);

//         if (ib_query->out_array) {
//             memcpy(&res->out_array, ib_query->out_array, sizeof(ibase_array)*ib_query->out_array_cnt);
//         }
//         *ib_resultp = res;
//     }

//     if (ib_query->in_sqlda) { /* has placeholders */
//         IBDEBUG("Query wants XSQLDA for input");
//         in_sqlda = emalloc(XSQLDA_LENGTH(ib_query->in_sqlda->sqld));
//         memcpy(in_sqlda, ib_query->in_sqlda, XSQLDA_LENGTH(ib_query->in_sqlda->sqld));
//         bind_buf = safe_emalloc(sizeof(BIND_BUF), ib_query->in_sqlda->sqld, 0);
//         if (_php_firebird_bind(in_sqlda, args, bind_buf, ib_query) == FAILURE) {
//             IBDEBUG("Could not bind input XSQLDA");
//             goto _php_firebird_exec_error;
//         }
//     }

//     if (ib_query->statement_type == isc_info_sql_stmt_exec_procedure) {
//         isc_result = isc_dsql_execute2(IB_STATUS, &ib_query->trans->handle,
//             &ib_query->stmt, SQLDA_CURRENT_VERSION, in_sqlda, out_sqlda);
//     } else {
//         isc_result = isc_dsql_execute(IB_STATUS, &ib_query->trans->handle,
//             &ib_query->stmt, SQLDA_CURRENT_VERSION, in_sqlda);
//     }
//     if (isc_result) {
//         IBDEBUG("Could not execute query");
//         _php_firebird_error();
//         goto _php_firebird_exec_error;
//     }
//     ib_query->trans->affected_rows = 0;

//     switch (ib_query->statement_type) {

//         unsigned long affected_rows;

//         case isc_info_sql_stmt_insert:
//         case isc_info_sql_stmt_update:
//         case isc_info_sql_stmt_delete:
//         case isc_info_sql_stmt_exec_procedure:

//             if (isc_dsql_sql_info(IB_STATUS, &ib_query->stmt, sizeof(info_count),
//                     info_count, sizeof(result), result)) {
//                 _php_firebird_error();
//                 goto _php_firebird_exec_error;
//             }

//             affected_rows = 0;

//             if (result[0] == isc_info_sql_records) {
//                 unsigned i = 3, result_size = isc_vax_integer(&result[1],2);

//                 while (result[i] != isc_info_end && i < result_size) {
//                     short len = (short)isc_vax_integer(&result[i+1],2);
//                     if (result[i] != isc_info_req_select_count) {
//                         affected_rows += isc_vax_integer(&result[i+3],len);
//                     }
//                     i += len+3;
//                 }
//             }

//             ib_query->trans->affected_rows = affected_rows;

//             if (!ib_query->out_sqlda) { /* no result set is being returned */
//                 if (affected_rows) {
//                     RETVAL_LONG(affected_rows);
//                 } else {
//                     RETVAL_TRUE;
//                 }
//                 break;
//             }
//         default:
//             RETVAL_TRUE;
//     }

//     rv = SUCCESS;

// _php_firebird_exec_error:

//     if (in_sqlda) {
//         efree(in_sqlda);
//     }
//     if (bind_buf)
//         efree(bind_buf);

//     if (rv == FAILURE) {
//         if (*ib_resultp) {
//             efree(*ib_resultp);
//             *ib_resultp = NULL;
//         }
//         if (out_sqlda) {
//             _php_firebird_free_xsqlda(out_sqlda);
//         }
//     }

//     return rv;
// }

static void _php_firebird_alloc_xsqlda(XSQLDA *sqlda)
{
    int i;

    for (i = 0; i < sqlda->sqld; i++) {
        XSQLVAR *var = &sqlda->sqlvar[i];

        switch (var->sqltype & ~1) {
            case SQL_TEXT:
                var->sqldata = safe_emalloc(sizeof(char), var->sqllen, 0);
                break;
            case SQL_VARYING:
                var->sqldata = safe_emalloc(sizeof(char), var->sqllen + sizeof(short), 0);
                break;
#ifdef SQL_BOOLEAN
            case SQL_BOOLEAN:
                var->sqldata = emalloc(sizeof(FB_BOOLEAN));
                break;
#endif
            case SQL_SHORT:
                var->sqldata = emalloc(sizeof(short));
                break;
            case SQL_LONG:
                var->sqldata = emalloc(sizeof(ISC_LONG));
                break;
            case SQL_FLOAT:
                var->sqldata = emalloc(sizeof(float));
                    break;
            case SQL_DOUBLE:
                var->sqldata = emalloc(sizeof(double));
                break;
            case SQL_INT64:
                var->sqldata = emalloc(sizeof(ISC_INT64));
                break;
            case SQL_TIMESTAMP:
                var->sqldata = emalloc(sizeof(ISC_TIMESTAMP));
                break;
            case SQL_TYPE_DATE:
                var->sqldata = emalloc(sizeof(ISC_DATE));
                break;
            case SQL_TYPE_TIME:
                var->sqldata = emalloc(sizeof(ISC_TIME));
                break;
            case SQL_BLOB:
            case SQL_ARRAY:
                var->sqldata = emalloc(sizeof(ISC_QUAD));
                break;
#if FB_API_VER >= 40
            // These are converted to VARCHAR via isc_dpb_set_bind tag at connect
            // case SQL_DEC16:
            // case SQL_DEC34:
            // case SQL_INT128:
            case SQL_TIMESTAMP_TZ:
                var->sqldata = emalloc(sizeof(ISC_TIMESTAMP_TZ));
                break;
            case SQL_TIME_TZ:
                var->sqldata = emalloc(sizeof(ISC_TIME_TZ));
                break;
#endif
            default:
                php_error(E_WARNING, "Unhandled sqltype: %d for sqlname %s %s:%d", var->sqltype, var->sqlname, __FILE__, __LINE__);
                break;
        } /* switch */

        if (var->sqltype & 1) { /* sql NULL flag */
            var->sqlind = emalloc(sizeof(short));
        } else {
            var->sqlind = NULL;
        }
    } /* for */
}
