#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Transaction_ce;
static zend_object_handlers FireBird_Transaction_object_handlers;

// static int alloc_array(firebird_array **ib_arrayp, unsigned short *array_cnt, XSQLDA *sqlda, zend_object *obj);
static void _php_firebird_alloc_xsqlda(XSQLDA *sqlda);
static void _php_firebird_process_trans(INTERNAL_FUNCTION_PARAMETERS, int commit);
static void _php_firebird_populate_tpb(zend_long trans_argl, zend_long trans_timeout, char *last_tpb, unsigned short *len);
static  int _php_firebird_prepare(INTERNAL_FUNCTION_PARAMETERS, const ISC_SCHAR *sql, zval *zstmt);

#define ROLLBACK    0
#define COMMIT      1
#define RETAIN      2

void transaction_ctor(zval *this, zval *connection, zend_long trans_args, zend_long lock_timeout)
{
    zend_update_property(FireBird_Transaction_ce, O_SET(this, connection));
    zend_update_property_long(FireBird_Transaction_ce, O_SET(this, trans_args));
    zend_update_property_long(FireBird_Transaction_ce, O_SET(this, lock_timeout));

    firebird_connection *conn = Z_CONNECTION_P(connection);
    firebird_trans *tr = Z_TRANSACTION_P(this);
    tr->db_handle = conn->db_handle;
}

PHP_METHOD(Transaction, __construct) {
    zval *connection;
    zend_long trans_args, lock_timeout;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_OBJECT_OF_CLASS(connection, FireBird_Connection_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(trans_args)
        Z_PARAM_LONG(lock_timeout)
    ZEND_PARSE_PARAMETERS_END();

    transaction_ctor(ZEND_THIS, connection, trans_args, lock_timeout);
}

int transaction_start(zval *tr_o)
{
    zval rv, *val;
    zend_long trans_args = 0, lock_timeout = 0;

    ISC_STATUS_ARRAY status;
    ISC_STATUS result;
    char tpb[TPB_MAX_SIZE];
    unsigned short tpb_len = 0;

    firebird_trans *tr = Z_TRANSACTION_P(tr_o);

    val = zend_read_property(FireBird_Transaction_ce, O_GET(tr_o, trans_args), 1, &rv);
    trans_args = Z_LVAL_P(val);

    val = zend_read_property(FireBird_Transaction_ce, O_GET(tr_o, lock_timeout), 1, &rv);
    lock_timeout = Z_LVAL_P(val);

    php_printf("trans_args=%d, lock_timeout=%d\n", trans_args, lock_timeout);

    _php_firebird_populate_tpb(trans_args, lock_timeout, tpb, &tpb_len);
    if(isc_start_transaction(status, &tr->tr_handle, 1, &tr->db_handle, tpb_len, tpb)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(tr_o));
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(Transaction, start) {
    ZEND_PARSE_PARAMETERS_NONE();

    if(FAILURE == transaction_start(ZEND_THIS)) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(Transaction, commit) {
    ZEND_PARSE_PARAMETERS_NONE();
    _php_firebird_process_trans(INTERNAL_FUNCTION_PARAM_PASSTHRU, COMMIT);
}

PHP_METHOD(Transaction, commit_ret) {
    ZEND_PARSE_PARAMETERS_NONE();
    _php_firebird_process_trans(INTERNAL_FUNCTION_PARAM_PASSTHRU, COMMIT | RETAIN);
}

PHP_METHOD(Transaction, rollback) {
    ZEND_PARSE_PARAMETERS_NONE();
    _php_firebird_process_trans(INTERNAL_FUNCTION_PARAM_PASSTHRU, ROLLBACK);
}

PHP_METHOD(Transaction, rollback_ret) {
    ZEND_PARSE_PARAMETERS_NONE();
    _php_firebird_process_trans(INTERNAL_FUNCTION_PARAM_PASSTHRU, ROLLBACK | RETAIN);
}

PHP_METHOD(Transaction, prepare)
{
    zval rv;
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == _php_firebird_prepare(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZSTR_VAL(sql), &rv)) {
        RETURN_FALSE;
    }

    RETVAL_OBJ(Z_OBJ(rv));
}

PHP_METHOD(Transaction, query)
{
    zval rv, *bind_args;
    uint32_t num_bind_args;
    zend_string *sql;

    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);
    ISC_STATUS_ARRAY status;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(sql)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == _php_firebird_prepare(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZSTR_VAL(sql), &rv)) {
        RETURN_FALSE;
    }

    zend_object *stmt_o = Z_OBJ(rv);

    if (FAILURE == _php_firebird_execute(status, bind_args, num_bind_args, stmt_o)) {
        zval_ptr_dtor(&rv);
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
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
    PHP_ME(Transaction, prepare, arginfo_FireBird_Transaction_prepare, ZEND_ACC_PUBLIC)
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

    DECLARE_PROP_OBJ(FireBird_Transaction_ce, connection, FireBird\\Connection, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_INT(FireBird_Transaction_ce, trans_args, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_INT(FireBird_Transaction_ce, lock_timeout, ZEND_ACC_PROTECTED_SET);
    ADD_ERR_PROPS(FireBird_Transaction_ce);

    FireBird_Transaction_ce->create_object = FireBird_Transaction_create;
    FireBird_Transaction_ce->default_object_handlers = &FireBird_Transaction_object_handlers;

    memcpy(&FireBird_Transaction_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Transaction_object_handlers.offset = XtOffsetOf(firebird_trans, std);
    FireBird_Transaction_object_handlers.free_obj = FireBird_Transaction_free_obj;
}

static void _php_firebird_populate_tpb(zend_long trans_argl, zend_long trans_timeout, char *last_tpb, unsigned short *len)
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

        // XXX: Engine should allocate this for outbound XSQLDA? No?
        if (var->sqltype & 1) { /* sql NULL flag */
            var->sqlind = emalloc(sizeof(short));
        } else {
            var->sqlind = NULL;
        }
    } /* for */
}

static void _php_firebird_process_trans(INTERNAL_FUNCTION_PARAMETERS, int commit)
{
    zval rv, *val;
    ISC_STATUS result;
    ISC_STATUS_ARRAY status;
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);

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
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        RETVAL_FALSE;
    } else {
        RETVAL_TRUE;
    }
}

int _php_firebird_bind(ISC_STATUS_ARRAY status, XSQLDA *sqlda, zval *b_vars, zend_object *stmt_o)
{
    int i, array_cnt = 0, rv = SUCCESS;
    firebird_stmt *stmt = Z_STMT_O(stmt_o);

    if(sqlda->sqld > 0) {
        // In case of repeated calls to execute()
        if(!stmt->bind_buf) {
            stmt->bind_buf = safe_emalloc(sizeof(firebird_bind_buf), stmt->in_sqlda->sqld, 0);
        }
    }

    for (i = 0; i < sqlda->sqld; ++i) {
        zval *b_var = &b_vars[i];
        XSQLVAR *var = &sqlda->sqlvar[i];

        var->sqlind = &stmt->bind_buf[i].sqlind;

        // XXX: Just pass NULL if you need NULL. No?
        if (Z_TYPE_P(b_var) == IS_NULL) {
            stmt->bind_buf[i].sqlind = -1;
            sqlda->sqlvar->sqldata = NULL;
            if (var->sqltype & SQL_ARRAY) ++array_cnt;
            continue;
        }

        /* check if a NULL should be inserted */
//         switch (Z_TYPE_P(b_var)) {
//             int force_null;

//             case IS_STRING:

//                 force_null = 0;

//                 /* for these types, an empty string can be handled like a NULL value */
//                 switch (var->sqltype & ~1) {
//                     case SQL_SHORT:
//                     case SQL_LONG:
//                     case SQL_INT64:
//                     case SQL_FLOAT:
//                     case SQL_DOUBLE:
//                     case SQL_TIMESTAMP:
//                     case SQL_TYPE_DATE:
//                     case SQL_TYPE_TIME:
// #if FB_API_VER >= 40
//                     case SQL_INT128:
//                     case SQL_DEC16:
//                     case SQL_DEC34:
//                     case SQL_TIMESTAMP_TZ:
//                     case SQL_TIME_TZ:
// #endif
//                         force_null = (Z_STRLEN_P(b_var) == 0);
//                 }

//                 if (! force_null) break;

//             case IS_NULL:
//                 buf[i].sqlind = -1;
//                 sqlda->sqlvar->sqldata = NULL;

//                 if (var->sqltype & SQL_ARRAY) ++array_cnt;

//                 continue;
//         }

        /* if we make it to this point, we must provide a value for the parameter */

        stmt->bind_buf[i].sqlind = 0;

        var->sqldata = (void*)&stmt->bind_buf[i].val;

        switch (var->sqltype & ~1) {
            struct tm t;

            case SQL_TIMESTAMP:
            // TODO:
            // case SQL_TIMESTAMP_TZ:
            // case SQL_TIME_TZ:
            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
                if (Z_TYPE_P(b_var) == IS_LONG) {
                    struct tm *res;
                    res = php_gmtime_r(&Z_LVAL_P(b_var), &t);
                    if (!res) {
                        return FAILURE;
                    }
                } else {
#ifdef HAVE_STRPTIME
                    char *format = INI_STR("firebird.timestampformat");

                    convert_to_string(b_var);

                    switch (var->sqltype & ~1) {
                        case SQL_TYPE_DATE:
                            format = INI_STR("firebird.dateformat");
                            break;
                        case SQL_TYPE_TIME:
                        // TODO:
                        // case SQL_TIME_TZ:
                            format = INI_STR("firebird.timeformat");
                    }
                    if (!strptime(Z_STRVAL_P(b_var), format, &t)) {
                        /* strptime() cannot handle it, so let IB have a try */
                        break;
                    }
#else /* ifndef HAVE_STRPTIME */
                    break; /* let IB parse it as a string */
#endif
                }

                switch (var->sqltype & ~1) {
                    default: /* == case SQL_TIMESTAMP */
                        isc_encode_timestamp(&t, &stmt->bind_buf[i].val.tsval);
                        break;
                    case SQL_TYPE_DATE:
                        isc_encode_sql_date(&t, &stmt->bind_buf[i].val.dtval);
                        break;
                    case SQL_TYPE_TIME:
                    // TODO:
                    // case SQL_TIME_TZ:
                        isc_encode_sql_time(&t, &stmt->bind_buf[i].val.tmval);
                        break;
                }
                continue;

            case SQL_BLOB:
                convert_to_string(b_var);

                if (Z_STRLEN_P(b_var) != BLOB_ID_LEN ||
                    !_php_firebird_string_to_quad(Z_STRVAL_P(b_var), &stmt->bind_buf[i].val.qval)) {

                    firebird_blob ib_blob = { 0, BLOB_INPUT };

                    if (isc_create_blob(status, &stmt->db_handle, &stmt->tr_handle, &ib_blob.bl_handle, &ib_blob.bl_qd)) {
                        return FAILURE;
                    }

                    if (_php_firebird_blob_add(status, b_var, &ib_blob) != SUCCESS) {
                        return FAILURE;
                    }

                    if (isc_close_blob(status, &ib_blob.bl_handle)) {
                        return FAILURE;
                    }

                    stmt->bind_buf[i].val.qval = ib_blob.bl_qd;
                }
                continue;
#ifdef SQL_BOOLEAN
            case SQL_BOOLEAN:

                switch (Z_TYPE_P(b_var)) {
                    case IS_LONG:
                    case IS_DOUBLE:
                    case IS_TRUE:
                    case IS_FALSE:
                        *(FB_BOOLEAN *)var->sqldata = zend_is_true(b_var) ? FB_TRUE : FB_FALSE;
                        break;
                    case IS_STRING:
                    {
                        zend_long lval;
                        double dval;

                        if ((Z_STRLEN_P(b_var) == 0)) {
                            *(FB_BOOLEAN *)var->sqldata = FB_FALSE;
                            break;
                        }

                        switch (is_numeric_string(Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), &lval, &dval, 0)) {
                            case IS_LONG:
                                *(FB_BOOLEAN *)var->sqldata = (lval != 0) ? FB_TRUE : FB_FALSE;
                                break;
                            case IS_DOUBLE:
                                *(FB_BOOLEAN *)var->sqldata = (dval != 0) ? FB_TRUE : FB_FALSE;
                                break;
                            default:
                                if (!zend_binary_strncasecmp(Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), "true", 4, 4)) {
                                    *(FB_BOOLEAN *)var->sqldata = FB_TRUE;
                                } else if (!zend_binary_strncasecmp(Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), "false", 5, 5)) {
                                    *(FB_BOOLEAN *)var->sqldata = FB_FALSE;
                                } else {
                                    _php_firebird_module_error("Parameter %d: cannot convert string to boolean", i+1);
                                    rv = FAILURE;
                                    continue;
                                }
                        }
                        break;
                    }
                    case IS_NULL:
                        stmt->bind_buf[i].sqlind = -1;
                        break;
                    default:
                        _php_firebird_module_error("Parameter %d: must be boolean", i+1);
                        rv = FAILURE;
                        continue;
                }
                var->sqltype = SQL_BOOLEAN;
                continue;
#endif
            case SQL_ARRAY:
                assert(false && "TODO: bind SQL_ARRAY");
                // if (Z_TYPE_P(b_var) != IS_ARRAY) {
                //     convert_to_string(b_var);

                //     if (Z_STRLEN_P(b_var) != BLOB_ID_LEN ||
                //         !_php_firebird_string_to_quad(Z_STRVAL_P(b_var), &buf[i].val.qval)) {

                //         _php_firebird_module_error("Parameter %d: invalid array ID",i+1);
                //         rv = FAILURE;
                //     }
                // } else {
                //     /* convert the array data into something IB can understand */
                //     ibase_array *ar = &ib_query->in_array[array_cnt];
                //     void *array_data = emalloc(ar->ar_size);
                //     ISC_QUAD array_id = { 0, 0 };

                //     if (FAILURE == _php_firebird_bind_array(b_var, array_data, ar->ar_size,
                //             ar, 0)) {
                //         _php_firebird_module_error("Parameter %d: failed to bind array argument", i+1);
                //         efree(array_data);
                //         rv = FAILURE;
                //         continue;
                //     }

                //     if (isc_array_put_slice(IB_STATUS, &ib_query->link->handle, &ib_query->trans->handle,
                //             &array_id, &ar->ar_desc, array_data, &ar->ar_size)) {
                //         _php_firebird_error();
                //         efree(array_data);
                //         return FAILURE;
                //     }
                //     buf[i].val.qval = array_id;
                //     efree(array_data);
                // }
                // ++array_cnt;
                continue;
        } /* switch */

        /* we end up here if none of the switch cases handled the field */
        convert_to_string(b_var);
        var->sqldata = Z_STRVAL_P(b_var);
        var->sqllen	 = Z_STRLEN_P(b_var);
        var->sqltype = SQL_TEXT;
    } /* for */
    return rv;
}

static int _php_firebird_prepare(INTERNAL_FUNCTION_PARAMETERS, const ISC_SCHAR *sql, zval *zstmt)
{
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);
    ISC_STATUS_ARRAY status;

    object_init_ex(zstmt, FireBird_Statement_ce);

    zend_object *stmt_o = Z_OBJ_P(zstmt);
    firebird_stmt *stmt = Z_STMT_O(stmt_o);

    stmt->db_handle = tr->db_handle;
    stmt->tr_handle = tr->tr_handle;

    if (isc_dsql_allocate_statement(status, &stmt->db_handle, &stmt->stmt_handle)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(zstmt);
        return FAILURE;
    }

    stmt->out_sqlda = (XSQLDA *) emalloc(XSQLDA_LENGTH(1));
    stmt->out_sqlda->sqln = 1;
    stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;

    if (isc_dsql_prepare(status, &stmt->tr_handle, &stmt->stmt_handle, 0, sql, SQL_DIALECT_CURRENT, stmt->out_sqlda)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(zstmt);
        return FAILURE;
    }

    if (stmt->out_sqlda->sqld > stmt->out_sqlda->sqln) {
        stmt->out_sqlda = erealloc(stmt->out_sqlda, XSQLDA_LENGTH(stmt->out_sqlda->sqld));
        stmt->out_sqlda->sqln = stmt->out_sqlda->sqld;
        stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;
        if (isc_dsql_describe(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->out_sqlda)) {
            update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
            zval_ptr_dtor(zstmt);
            return FAILURE;
        }
    }

    _php_firebird_alloc_xsqlda(stmt->out_sqlda);

    /* maybe have input placeholders? */
    stmt->in_sqlda = emalloc(XSQLDA_LENGTH(1));
    stmt->in_sqlda->sqln = 1;
    stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;
    if (isc_dsql_describe_bind(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(zstmt);
        return FAILURE;
    }

    /* not enough input variables ? */
    if (stmt->in_sqlda->sqln < stmt->in_sqlda->sqld) {
        stmt->in_sqlda = erealloc(stmt->in_sqlda, XSQLDA_LENGTH(stmt->in_sqlda->sqld));
        stmt->in_sqlda->sqln = stmt->in_sqlda->sqld;
        stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;

        if (isc_dsql_describe_bind(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
            update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
            zval_ptr_dtor(zstmt);
            return FAILURE;
        }
    }

    static char info_type[] = { isc_info_sql_stmt_type };
    char result[8];

    /* find out what kind of statement was prepared */
    if (isc_dsql_sql_info(status, &stmt->stmt_handle, sizeof(info_type), info_type, sizeof(result), result)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(zstmt);
        return FAILURE;
    }

    stmt->statement_type = result[3];

    return SUCCESS;
}
