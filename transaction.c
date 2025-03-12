#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Transaction_ce;
static zend_object_handlers FireBird_Transaction_object_handlers;

// static int alloc_array(firebird_array **ib_arrayp, unsigned short *array_cnt, XSQLDA *sqlda, zend_object *obj);
static void _php_firebird_process_trans(INTERNAL_FUNCTION_PARAMETERS, int commit);
static void _php_firebird_populate_tpb(zend_long trans_argl, zend_long trans_timeout, char *last_tpb, unsigned short *len);

#define ROLLBACK    0
#define COMMIT      1
#define RETAIN      2

void transaction_ctor(zval *tr_o, zval *connection, zend_long trans_args, zend_long lock_timeout)
{
    zend_update_property(FireBird_Transaction_ce, O_SET(tr_o, connection));
    zend_update_property_long(FireBird_Transaction_ce, O_SET(tr_o, trans_args));
    zend_update_property_long(FireBird_Transaction_ce, O_SET(tr_o, lock_timeout));

    firebird_connection *conn = Z_CONNECTION_P(connection);
    firebird_trans *tr = Z_TRANSACTION_P(tr_o);
    tr->db_handle = &conn->db_handle;
}

PHP_METHOD(Transaction, __construct) {
    zval *connection = NULL;
    zend_long trans_args = 0, lock_timeout = 0;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_OBJECT_OF_CLASS(connection, FireBird_Connection_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(trans_args)
        Z_PARAM_LONG(lock_timeout)
    ZEND_PARSE_PARAMETERS_END();

    transaction_ctor(ZEND_THIS, connection, trans_args, lock_timeout);
}

int transaction_start(ISC_STATUS_ARRAY status, zval *tr_o)
{
    zval rv;
    zval *trans_args = NULL, *lock_timeout = NULL;
    ISC_STATUS result;
    char tpb[TPB_MAX_SIZE];
    unsigned short tpb_len = 0;

    trans_args = zend_read_property(FireBird_Transaction_ce, O_GET(tr_o, trans_args), 1, &rv);
    lock_timeout = zend_read_property(FireBird_Transaction_ce, O_GET(tr_o, lock_timeout), 1, &rv);

    FBDEBUG("trans_args=%d, lock_timeout=%d", Z_LVAL_P(trans_args), Z_LVAL_P(lock_timeout));

    _php_firebird_populate_tpb(Z_LVAL_P(trans_args), Z_LVAL_P(lock_timeout), tpb, &tpb_len);

    firebird_trans *tr = Z_TRANSACTION_P(tr_o);

    if(isc_start_transaction(status, &tr->tr_handle, 1, tr->db_handle, tpb_len, tpb)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(Transaction, start) {
    ISC_STATUS_ARRAY status;

    ZEND_PARSE_PARAMETERS_NONE();

    if(FAILURE == transaction_start(status, ZEND_THIS)) {
        update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
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

const zend_function_entry FireBird_Transaction_methods[] = {
    PHP_ME(Transaction, __construct, arginfo_FireBird_Transaction_construct, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, start, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, commit, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, commit_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, rollback, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, rollback_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
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
    FBDEBUG("FireBird_Transaction_free_obj");

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
    DECLARE_PROP_LONG(FireBird_Transaction_ce, trans_args, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Transaction_ce, lock_timeout, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Transaction_ce);

    zend_class_implements(FireBird_Transaction_ce, 1, FireBird_IError_ce);

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
        update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
        RETVAL_FALSE;
    } else {
        RETVAL_TRUE;
    }
}
