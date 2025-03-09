#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Query_ce;
static zend_object_handlers FireBird_Query_object_handlers;

// static int alloc_array(firebird_array **ib_arrayp, unsigned short *array_cnt, XSQLDA *sqlda, zend_object *obj);

PHP_METHOD(Query, __construct)
{
    zval *transaction;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(transaction, FireBird_Transaction_ce)
    ZEND_PARSE_PARAMETERS_END();

    zend_update_property(FireBird_Query_ce, Z_OBJ_P(ZEND_THIS), "transaction", sizeof("transaction") - 1, transaction);

    firebird_trans *tr = Z_TRANSACTION_P(transaction);
    firebird_query *q = Z_QUERY_P(ZEND_THIS);
    q->db_handle = tr->db_handle;
    q->tr_handle = tr->tr_handle;
}

PHP_METHOD(Query, query)
{
    zval rv, *val;
    zend_long trans_args = 0, lock_timeout = 0;
    ISC_STATUS_ARRAY status;
    char tpb[TPB_MAX_SIZE];
    unsigned short tpb_len = 0;
    zend_string *sql = NULL;

    // val = zend_read_property(FireBird_Query_ce, Z_OBJ_P(ZEND_THIS), "transaction", sizeof("transaction") - 1, 0, &rv);
    // firebird_trans *tr = Z_TRANSACTION_P(val);
    firebird_query *q = Z_QUERY_P(ZEND_THIS);

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

    // zval *v = emalloc(sizeof(zval));
    zval v;
    object_init_ex(&v, FireBird_Statement_ce);
    zend_object *stmt_o = Z_OBJ(v);
    firebird_stmt *stmt = Z_STMT_O(stmt_o);

    stmt->db_handle = q->db_handle;
    stmt->tr_handle = q->tr_handle;
    stmt->has_more_rows = 1;

    if (isc_dsql_allocate_statement(status, &stmt->db_handle, &stmt->stmt_handle)) {
        update_err_props(status, FireBird_Query_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&v);
        RETURN_FALSE;
    }

    stmt->out_sqlda = (XSQLDA *) emalloc(XSQLDA_LENGTH(1));
    stmt->out_sqlda->sqln = 1;
    stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;

    if (isc_dsql_prepare(status, &stmt->tr_handle, &stmt->stmt_handle, 0, ZSTR_VAL(sql), SQL_DIALECT_CURRENT, stmt->out_sqlda)) {
        update_err_props(status, FireBird_Query_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&v);
        RETURN_FALSE;
    }

    static char info_type[] = { isc_info_sql_stmt_type };
    char result[8];

    /* find out what kind of statement was prepared */
    if (isc_dsql_sql_info(status, &stmt->stmt_handle, sizeof(info_type), info_type, sizeof(result), result)) {
        update_err_props(status, FireBird_Query_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&v);
        RETURN_FALSE;
    }

    stmt->statement_type = result[3];

    if (stmt->out_sqlda->sqld > stmt->out_sqlda->sqln) {
        stmt->out_sqlda = erealloc(stmt->out_sqlda, XSQLDA_LENGTH(stmt->out_sqlda->sqld));
        stmt->out_sqlda->sqln = stmt->out_sqlda->sqld;
        stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;
        if (isc_dsql_describe(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->out_sqlda)) {
            update_err_props(status, FireBird_Query_ce, Z_OBJ_P(ZEND_THIS));
            zval_ptr_dtor(&v);
            RETURN_FALSE;
        }
    }

    /* maybe have input placeholders? */
    stmt->in_sqlda = emalloc(XSQLDA_LENGTH(1));
    stmt->in_sqlda->sqln = 1;
    stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;
    if (isc_dsql_describe_bind(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
        update_err_props(status, FireBird_Query_ce, Z_OBJ_P(ZEND_THIS));
        zval_ptr_dtor(&v);
        RETURN_FALSE;
    }

    /* not enough input variables ? */
    if (stmt->in_sqlda->sqln < stmt->in_sqlda->sqld) {
        stmt->in_sqlda = erealloc(stmt->in_sqlda, XSQLDA_LENGTH(stmt->in_sqlda->sqld));
        stmt->in_sqlda->sqln = stmt->in_sqlda->sqld;
        stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;

        if (isc_dsql_describe_bind(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
            update_err_props(status, FireBird_Query_ce, Z_OBJ_P(ZEND_THIS));
            zval_ptr_dtor(&v);
            RETURN_FALSE;
        }
    }

    zval_ptr_dtor(&v);
    RETVAL_OBJ(stmt_o);

    // TODO: handle SQL_ARRAY

    /* no, haven't placeholders at all */
    // if (q->in_sqlda->sqld == 0) {
    //     efree(q->in_sqlda);
    //     q->in_sqlda = NULL;
    // // } else if (FAILURE == alloc_arraqy(&q->in_array, &q->in_array_cnt, q->in_sqlda, Z_OBJ_P(ZEND_THIS))) {
    // //     // update_err_props(status, FireBird_Query_ce, Z_OBJ_P(ZEND_THIS));
    // //     goto query_error;
    // }

    // if (q->out_sqlda->sqld == 0) {
    //     efree(q->out_sqlda);
    //     q->out_sqlda = NULL;
    // // } else if (FAILURE == alloc_array(&q->out_array, &q->out_array_cnt, q->out_sqlda, Z_OBJ_P(ZEND_THIS))) {
    // //     // update_err_props(status, FireBird_Query_ce, Z_OBJ_P(ZEND_THIS));
    // //     goto query_error;
    // }
}

const zend_function_entry FireBird_Query_methods[] = {
    PHP_ME(Query, __construct, arginfo_FireBird_Query_construct, ZEND_ACC_PUBLIC)
    PHP_ME(Query, query, arginfo_FireBird_Query_query, ZEND_ACC_PUBLIC)

    PHP_FE_END
};

static zend_object *FireBird_Query_create(zend_class_entry *ce)
{
    firebird_query *q = zend_object_alloc(sizeof(firebird_query), ce);

    zend_object_std_init(&q->std, ce);
    object_properties_init(&q->std, ce);

    return &q->std;
}

static void FireBird_Query_free_obj(zend_object *obj)
{
    php_printf("FireBird_Query_free_obj\n");
    firebird_query *q = Z_QUERY_O(obj);

    zend_object_std_dtor(&q->std);
}

void register_FireBird_Query_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Query", FireBird_Query_methods);
    FireBird_Query_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Query_ce, transaction, FireBird\\Transaction, ZEND_ACC_PROTECTED);
    ADD_ERR_PROPS(FireBird_Query_ce);

    FireBird_Query_ce->create_object = FireBird_Query_create;
    FireBird_Query_ce->default_object_handlers = &FireBird_Query_object_handlers;

    memcpy(&FireBird_Query_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Query_object_handlers.offset = XtOffsetOf(firebird_query, std);
    FireBird_Query_object_handlers.free_obj = FireBird_Query_free_obj;
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
//                 update_err_props(status, FireBird_Query_ce, obj);
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

