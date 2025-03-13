#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ibase.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "ext/spl/spl_exceptions.h"

#include "php_firebird_includes.h"
#include "pdo_firebird_utils.h"

zend_class_entry *FireBird_Statement_ce;
static zend_object_handlers FireBird_Statement_object_handlers;

static void _php_firebird_alloc_xsqlda(XSQLDA *sqlda);
static void _php_firebird_free_xsqlda(XSQLDA *sqlda);
static void _php_firebird_free_stmt(firebird_stmt *s);
static void _php_firebird_fetch_hash(INTERNAL_FUNCTION_PARAMETERS, int fetch_type);
static int statement_execute(ISC_STATUS_ARRAY status, zval *stmt_o, zval *bind_args, uint32_t num_bind_args);
static int statement_bind(ISC_STATUS_ARRAY status, zval *stmt_o, XSQLDA *sqlda, zval *b_vars);

#define FETCH_ROW       1
#define FETCH_ARRAY     2

void statement_ctor(zval *stmt_o, zval *transaction)
{
    zend_update_property(FireBird_Statement_ce, O_SET(stmt_o, transaction));

    firebird_stmt *stmt = Z_STMT_P(stmt_o);
    firebird_trans *tr = Z_TRANSACTION_P(transaction);

    stmt->db_handle = tr->db_handle;
    stmt->tr_handle = &tr->tr_handle;
}

PHP_METHOD(Statement, __construct)
{
    // zval *transaction = NULL;

    // ZEND_PARSE_PARAMETERS_START(1, 1)
    //     Z_PARAM_OBJECT_OF_CLASS(transaction, FireBird_Transaction_ce)
    // ZEND_PARSE_PARAMETERS_END();

    // statement_ctor(ZEND_THIS, transaction);
}

PHP_METHOD(Statement, fetch_row)
{
    _php_firebird_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, FETCH_ROW);
}

PHP_METHOD(Statement, fetch_array)
{
    _php_firebird_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, FETCH_ARRAY);
}

PHP_METHOD(Statement, fetch_object)
{
    _php_firebird_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, FETCH_ARRAY);

    if (Z_TYPE_P(return_value) == IS_ARRAY) {
        convert_to_object(return_value);
    }
}

PHP_METHOD(Statement, close)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_stmt *stmt = Z_STMT_P(ZEND_THIS);
    ISC_STATUS_ARRAY status;

    if (isc_dsql_free_statement(status, &stmt->stmt_handle, DSQL_close)) {
        update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(Statement, execute)
{
    ISC_STATUS_ARRAY status;
    zval *bind_args;
    uint32_t num_bind_args;

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == statement_execute(status, ZEND_THIS, bind_args, num_bind_args)) {
        update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

int statement_prepare(ISC_STATUS_ARRAY status, zval *stmt_o, const ISC_SCHAR *sql)
{
    firebird_stmt *stmt = Z_STMT_P(stmt_o);

    // TODO: configure auto-close, throw or warning
    if (stmt->stmt_handle > 0) {
        zend_throw_exception_ex(spl_ce_RuntimeException, 0, "Statement already active");
        return FAILURE;
    }

    if (isc_dsql_allocate_statement(status, stmt->db_handle, &stmt->stmt_handle)) {
        return FAILURE;
    }

    stmt->out_sqlda = (XSQLDA *) emalloc(XSQLDA_LENGTH(1));
    stmt->out_sqlda->sqln = 1;
    stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;

    if (isc_dsql_prepare(status, stmt->tr_handle, &stmt->stmt_handle, 0, sql, SQL_DIALECT_CURRENT, stmt->out_sqlda)) {
        return FAILURE;
    }

    if (stmt->out_sqlda->sqld > stmt->out_sqlda->sqln) {
        stmt->out_sqlda = erealloc(stmt->out_sqlda, XSQLDA_LENGTH(stmt->out_sqlda->sqld));
        stmt->out_sqlda->sqln = stmt->out_sqlda->sqld;
        stmt->out_sqlda->version = SQLDA_CURRENT_VERSION;
        if (isc_dsql_describe(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->out_sqlda)) {
            return FAILURE;
        }
    }

    _php_firebird_alloc_xsqlda(stmt->out_sqlda);

    /* maybe have input placeholders? */
    stmt->in_sqlda = emalloc(XSQLDA_LENGTH(1));
    stmt->in_sqlda->sqln = 1;
    stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;
    if (isc_dsql_describe_bind(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
        return FAILURE;
    }

    /* not enough input variables ? */
    if (stmt->in_sqlda->sqln < stmt->in_sqlda->sqld) {
        stmt->in_sqlda = erealloc(stmt->in_sqlda, XSQLDA_LENGTH(stmt->in_sqlda->sqld));
        stmt->in_sqlda->sqln = stmt->in_sqlda->sqld;
        stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;

        if (isc_dsql_describe_bind(status, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda)) {
            return FAILURE;
        }
    }

    static char info_type[] = { isc_info_sql_stmt_type };
    char result[8];

    /* find out what kind of statement was prepared */
    if (isc_dsql_sql_info(status, &stmt->stmt_handle, sizeof(info_type), info_type, sizeof(result), result)) {
        return FAILURE;
    }

    stmt->statement_type = result[3];

    return SUCCESS;
}

PHP_METHOD(Statement, query)
{
    zval rv, *bind_args;
    uint32_t num_bind_args;
    zend_string *sql;

    // firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);
    ISC_STATUS_ARRAY status;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(sql)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == statement_prepare(status, ZEND_THIS, ZSTR_VAL(sql))) {
        update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    if (FAILURE == statement_execute(status, ZEND_THIS, bind_args, num_bind_args)) {
        update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;

    // TODO: handle SQL_ARRAY

    /* no, haven't placeholders at all */
    // if (q->in_sqlda->sqld == 0) {
    //     efree(q->in_sqlda);
    //     q->in_sqlda = NULL;
    // // } else if (FAILURE == alloc_arraqy(&q->in_array, &q->in_array_cnt, q->in_sqlda, ZEND_THIS)) {
    // //     // update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
    // //     goto query_error;
    // }

    // if (q->out_sqlda->sqld == 0) {
    //     efree(q->out_sqlda);
    //     q->out_sqlda = NULL;
    // // } else if (FAILURE == alloc_array(&q->out_array, &q->out_array_cnt, q->out_sqlda,(ZEND_THIS)) {
    // //     // update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
    // //     goto query_error;
    // }
}

const zend_function_entry FireBird_Statement_methods[] = {
    PHP_ME(Statement, __construct, arginfo_FireBird_Statement_construct, ZEND_ACC_PRIVATE)
    PHP_ME(Statement, fetch_row, arginfo_FireBird_Statement_fetch_row, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, fetch_array, arginfo_FireBird_Statement_fetch_row, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, fetch_object, arginfo_FireBird_Statement_fetch_object, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, execute, arginfo_FireBird_Statement_execute, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, close, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    // PHP_ME(Statement, prepare, arginfo_FireBird_Statement_prepare, ZEND_ACC_PUBLIC)
    // PHP_ME(Statement, query, arginfo_FireBird_Statement_query, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Statement_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Statement_create()");

    firebird_stmt *s = zend_object_alloc(sizeof(firebird_stmt), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

// TODO: able to DSQL_drop from PHP
static void FireBird_Statement_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Statement_free_obj");

    firebird_stmt *stmt = Z_STMT_O(obj);

    if (stmt->stmt_handle) {
        ISC_STATUS_ARRAY status;
        if (isc_dsql_free_statement(status, &stmt->stmt_handle, DSQL_drop)) {
            // TODO: report errors?
        } else {
            stmt->stmt_handle = 0;
        }
    }

    _php_firebird_free_stmt(stmt);

    zend_object_std_dtor(&stmt->std);
}

void register_FireBird_Statement_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Statement", FireBird_Statement_methods);
    FireBird_Statement_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Statement_ce, transaction, FireBird\\Transaction, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Statement_ce);

    zend_class_implements(FireBird_Statement_ce, 1, FireBird_IError_ce);

    FireBird_Statement_ce->create_object = FireBird_Statement_create;
    FireBird_Statement_ce->default_object_handlers = &FireBird_Statement_object_handlers;

    memcpy(&FireBird_Statement_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Statement_object_handlers.offset = XtOffsetOf(firebird_stmt, std);
    FireBird_Statement_object_handlers.free_obj = FireBird_Statement_free_obj;
}

static void _php_firebird_free_stmt(firebird_stmt *s)
{
    if (s->in_sqlda) {
        efree(s->in_sqlda);
    }
    if (s->out_sqlda) {
        _php_firebird_free_xsqlda(s->out_sqlda);
    }
    if (s->in_array) {
        efree(s->in_array);
    }
    if (s->out_array) {
        efree(s->out_array);
    }
    if(s->bind_buf) {
        efree(s->bind_buf);
    }
}

static int _php_firebird_var_zval(zval *val, void *data, int type, int len, int scale, int flags)
{
    static ISC_INT64 const scales[] = { 1, 10, 100, 1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000,
        LL_LIT(10000000000),
        LL_LIT(100000000000),
        LL_LIT(1000000000000),
        LL_LIT(10000000000000),
        LL_LIT(100000000000000),
        LL_LIT(1000000000000000),
        LL_LIT(10000000000000000),
        LL_LIT(100000000000000000),
        LL_LIT(1000000000000000000)
    };

    switch (type & ~1) {
        unsigned short l;
        zend_long n;
        char string_data[255];
        struct tm t;
        char *format;

        case SQL_VARYING:
            len = ((firebird_vary *) data)->vary_length;
            data = ((firebird_vary *) data)->vary_string;
            /* no break */
        case SQL_TEXT:
            ZVAL_STRINGL(val, (char*)data, len);
            break;
#ifdef SQL_BOOLEAN
        case SQL_BOOLEAN:
            ZVAL_BOOL(val, *(FB_BOOLEAN *) data);
            break;
#endif
        case SQL_SHORT:
            n = *(short *) data;
            goto _sql_long;
        case SQL_INT64:
#if (SIZEOF_ZEND_LONG >= 8)
            n = *(zend_long *) data;
            goto _sql_long;
#else
            if (scale == 0) {
                l = slprintf(string_data, sizeof(string_data), "%" LL_MASK "d", *(ISC_INT64 *) data);
                ZVAL_STRINGL(val,string_data,l);
            } else {
                ISC_INT64 n = *(ISC_INT64 *) data, f = scales[-scale];

                if (n >= 0) {
                    l = slprintf(string_data, sizeof(string_data), "%" LL_MASK "d.%0*" LL_MASK "d", n / f, -scale, n % f);
                } else if (n <= -f) {
                    l = slprintf(string_data, sizeof(string_data), "%" LL_MASK "d.%0*" LL_MASK "d", n / f, -scale, -n % f);
                 } else {
                    l = slprintf(string_data, sizeof(string_data), "-0.%0*" LL_MASK "d", -scale, -n % f);
                }
                ZVAL_STRINGL(val,string_data,l);
            }
            break;
#endif
        case SQL_LONG:
            n = *(ISC_LONG *) data;
        _sql_long:
            if (scale == 0) {
                ZVAL_LONG(val,n);
            } else {
                zend_long f = (zend_long) scales[-scale];

                if (n >= 0) {
                    l = slprintf(string_data, sizeof(string_data), ZEND_LONG_FMT ".%0*" ZEND_LONG_FMT_SPEC, n / f, -scale,  n % f);
                } else if (n <= -f) {
                    l = slprintf(string_data, sizeof(string_data), ZEND_LONG_FMT ".%0*" ZEND_LONG_FMT_SPEC, n / f, -scale,  -n % f);
                } else {
                    l = slprintf(string_data, sizeof(string_data), "-0.%0*" ZEND_LONG_FMT_SPEC, -scale, -n % f);
                }
                ZVAL_STRINGL(val, string_data, l);
            }
            break;
        case SQL_FLOAT:
            ZVAL_DOUBLE(val, *(float *) data);
            break;
        case SQL_DOUBLE:
            ZVAL_DOUBLE(val, *(double *) data);
            break;
#if FB_API_VER >= 40
        // These are converted to VARCHAR via isc_dpb_set_bind tag at connect
        // case SQL_DEC16:
        // case SQL_DEC34:
        // case SQL_INT128:
        case SQL_TIME_TZ:
        case SQL_TIMESTAMP_TZ:
            char timeZoneBuffer[40] = {0};
            unsigned year, month, day, hours, minutes, seconds, fractions;

            if((type & ~1) == SQL_TIME_TZ){
                format = INI_STR("firebird.timeformat");
                fb_decode_time_tz((ISC_TIME_TZ *) data, &hours, &minutes, &seconds, &fractions, sizeof(timeZoneBuffer), timeZoneBuffer);
                ISC_TIME time = fb_encode_time(hours, minutes, seconds, fractions);
                isc_decode_sql_time(&time, &t);
            } else {
                format = INI_STR("firebird.timestampformat");
                fb_decode_timestamp_tz((ISC_TIMESTAMP_TZ *) data, &year, &month, &day, &hours, &minutes, &seconds, &fractions, sizeof(timeZoneBuffer), timeZoneBuffer);
                ISC_TIMESTAMP ts;
                ts.timestamp_date = fb_encode_date(year, month, day);
                ts.timestamp_time = fb_encode_time(hours, minutes, seconds, fractions);
                isc_decode_timestamp(&ts, &t);
            }

            if (flags & PHP_FIREBIRD_UNIXTIME) {
                ZVAL_LONG(val, mktime(&t));
            } else {
                char timeBuf[80] = {0};
                l = strftime(timeBuf, sizeof(timeBuf), format, &t);
                if (l == 0) {
                    return FAILURE;
                }

                size_t l = sprintf(string_data, "%s %s", timeBuf, timeZoneBuffer);
                ZVAL_STRINGL(val, string_data, l);
            }
            break;
#endif
        case SQL_DATE: /* == case SQL_TIMESTAMP: */
            format = INI_STR("firebird.timestampformat");
            isc_decode_timestamp((ISC_TIMESTAMP *) data, &t);
            goto format_date_time;
        case SQL_TYPE_DATE:
            format = INI_STR("firebird.dateformat");
            isc_decode_sql_date((ISC_DATE *) data, &t);
            goto format_date_time;
        case SQL_TYPE_TIME:
            format = INI_STR("firebird.timeformat");
            isc_decode_sql_time((ISC_TIME *) data, &t);

format_date_time:
            /*
              XXX - Might have to remove this later - seems that isc_decode_date()
               always sets tm_isdst to 0, sometimes incorrectly (InterBase 6 bug?)
            */
            t.tm_isdst = -1;
#if HAVE_STRUCT_TM_TM_ZONE
            t.tm_zone = tzname[0];
#endif
            if (flags & PHP_FIREBIRD_UNIXTIME) {
                ZVAL_LONG(val, mktime(&t));
            } else {
                l = strftime(string_data, sizeof(string_data), format, &t);
                ZVAL_STRINGL(val, string_data, l);
                break;
            }
    } /* switch (type) */
    return SUCCESS;
}

static void _php_firebird_fetch_hash(INTERNAL_FUNCTION_PARAMETERS, int fetch_type)
{
    zval *result_arg;
    zend_long flags = 0;
    zend_long i, array_cnt = 0;
    firebird_stmt *stmt = Z_STMT_P(ZEND_THIS);
    ISC_STATUS_ARRAY status;

    if (stmt->out_sqlda == NULL || !stmt->has_more_rows) {
        RETURN_NULL();
    }

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();

    if (stmt->statement_type != isc_info_sql_stmt_exec_procedure) {
        ISC_STATUS result = isc_dsql_fetch(status, &stmt->stmt_handle, 1, stmt->out_sqlda);
        if (result) {
            if (result == 100L) {
                stmt->has_more_rows = 0;
                RETURN_NULL();
            } else {
                update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
                RETURN_FALSE;
            }
        }
    } else {
        stmt->has_more_rows = 0;
    }

    array_init(return_value);

    for (i = 0; i < stmt->out_sqlda->sqld; ++i) {
        XSQLVAR *var = &stmt->out_sqlda->sqlvar[i];
        char buf[METADATALENGTH+4], *alias = var->aliasname;

        if (! (fetch_type & FETCH_ROW)) {
            int i = 0;
            char const *base = "FIELD"; /* use 'FIELD' if name is empty */

            /**
            * Ensure no two columns have identical names:
            * keep generating new names until we find one that is unique.
            */
            switch (*alias) {
                void *p;

                default:
                    i = 1;
                    base = alias;

                    while ((p = zend_symtable_str_find_ptr(
                            Z_ARRVAL_P(return_value), alias, strlen(alias))) != NULL) {

                case '\0':
                        snprintf(alias = buf, sizeof(buf), "%s_%02d", base, i++);
                    }
            }
        }

        if (((var->sqltype & 1) == 0) || *var->sqlind != -1) {
            zval result;

            switch (var->sqltype & ~1) {

                default:
                    _php_firebird_var_zval(&result, var->sqldata, var->sqltype, var->sqllen, var->sqlscale, flags);
                    break;
                case SQL_BLOB:
                    if (flags & PHP_FIREBIRD_FETCH_BLOBS) { /* fetch blob contents into hash */
                        firebird_blob blob_handle;
                        zend_ulong max_len = 0;
                        static char bl_items[] = { isc_info_blob_total_length };
                        char bl_info[20];
                        unsigned short i;

                        blob_handle.bl_handle = 0;
                        blob_handle.bl_qd = *(ISC_QUAD *) var->sqldata;

                        if (isc_open_blob(status, stmt->db_handle, stmt->tr_handle,
                            &blob_handle.bl_handle, &blob_handle.bl_qd)) {
                                update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
                                goto _php_firebird_fetch_error;
                        }

                        if (isc_blob_info(status, &blob_handle.bl_handle, sizeof(bl_items), bl_items,
                            sizeof(bl_info), bl_info)) {
                                update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
                                goto _php_firebird_fetch_error;
                        }

                        /* find total length of blob's data */
                        for (i = 0; i < sizeof(bl_info); ) {
                            unsigned short item_len;
                            char item = bl_info[i++];

                            if (item == isc_info_end || item == isc_info_truncated ||
                                item == isc_info_error || i >= sizeof(bl_info)) {
                                    _php_firebird_module_error("Could not determine BLOB size (internal error)");
                                    goto _php_firebird_fetch_error;
                            }

                            item_len = (unsigned short) isc_vax_integer(&bl_info[i], 2);

                            if (item == isc_info_blob_total_length) {
                                max_len = isc_vax_integer(&bl_info[i+2], item_len);
                                break;
                            }
                            i += item_len+2;
                        }

                        if (max_len == 0) {
                            ZVAL_STRING(&result, "");
                        } else if (SUCCESS != _php_firebird_blob_get(status, &result, &blob_handle, max_len)) {
                            update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
                            goto _php_firebird_fetch_error;
                        }

                        if (isc_close_blob(status, &blob_handle.bl_handle)) {
                            update_err_props(status, FireBird_Statement_ce, ZEND_THIS);
                            goto _php_firebird_fetch_error;
                        }

                    } else { /* blob id only */
                        ISC_QUAD bl_qd = *(ISC_QUAD *) var->sqldata;
                        ZVAL_NEW_STR(&result, _php_firebird_quad_to_string(bl_qd));
                    }
                    break;
                case SQL_ARRAY:
                    assert(false && "TODO: SQL_ARRAY");
                    // if (flag & PHP_FIREBIRD_FETCH_ARRAYS) { /* array can be *huge* so only fetch if asked */
                    //     ISC_QUAD ar_qd = *(ISC_QUAD *) var->sqldata;
                    //     ibase_array *ib_array = &stmt->out_array[array_cnt++];
                    //     void *ar_data = emalloc(ib_array->ar_size);

                    //     if (isc_array_get_slice(IB_STATUS, &stmt->link->handle,
                    //             &stmt->trans->handle, &ar_qd, &ib_array->ar_desc,
                    //             ar_data, &ib_array->ar_size)) {
                    //         _php_firebird_error();
                    //         efree(ar_data);
                    //         goto _php_firebird_fetch_error;
                    //     }

                    //     if (FAILURE == _php_firebird_arr_zval(&result, ar_data, ib_array->ar_size, ib_array,
                    //             0, flag)) {
                    //         efree(ar_data);
                    //         goto _php_firebird_fetch_error;
                    //     }
                    //     efree(ar_data);

                    // } else { /* blob id only */
                    //     ISC_QUAD ar_qd = *(ISC_QUAD *) var->sqldata;
                    //     ZVAL_NEW_STR(&result, _php_firebird_quad_to_string(ar_qd));
                    // }
                    break;
                _php_firebird_fetch_error:
                    zval_ptr_dtor_nogc(&result);
                    RETURN_FALSE;
            } /* switch */

            if (fetch_type & FETCH_ROW) {
                add_index_zval(return_value, i, &result);
            } else {
                add_assoc_zval(return_value, alias, &result);
            }
        } else {
            if (fetch_type & FETCH_ROW) {
                add_index_null(return_value, i);
            } else {
                add_assoc_null(return_value, alias);
            }
        }
    } /* for field */
}

static void _php_firebird_free_xsqlda(XSQLDA *sqlda)
{
    int i;
    XSQLVAR *var;

    if (sqlda) {
        var = sqlda->sqlvar;
        for (i = 0; i < min(sqlda->sqld, sqlda->sqln); i++, var++) {
            efree(var->sqldata);
            if (var->sqlind) { // XXX: should free for out sqlda or not?
                efree(var->sqlind);
            }
        }
        efree(sqlda);
    }
}

static int statement_execute(ISC_STATUS_ARRAY status, zval *stmt_o, zval *bind_args, uint32_t num_bind_args)
{
    firebird_stmt *stmt = Z_STMT_P(stmt_o);

    if (num_bind_args != stmt->in_sqlda->sqld) {
        zend_throw_exception_ex(zend_ce_argument_count_error, 0,
            "Statement expects %d arguments, %d given", stmt->in_sqlda->sqld, num_bind_args);
        return FAILURE;
    }

    /* has placeholders */
    if (stmt->in_sqlda->sqld > 0) {
        if (FAILURE == statement_bind(status, stmt_o, stmt->in_sqlda, bind_args)) {
            return FAILURE;
        }
    }

    ISC_STATUS isc_result;
    if (stmt->statement_type == isc_info_sql_stmt_exec_procedure) {
        assert(false && "TODO: isc_info_sql_stmt_exec_procedure");
        // isc_result = isc_dsql_execute2(IB_STATUS, &stmt->transtmt->handle,
        //     &stmt->stmt, SQLDA_CURRENT_VERSION, in_sqlda, out_sqlda);
    } else {
        FBDEBUG("isc_dsql_execute: %d", stmt->stmt_handle);
        isc_result = isc_dsql_execute(status, stmt->tr_handle, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda);
    }

    if (isc_result) {
        return FAILURE;
    }

    stmt->has_more_rows = 1;

    return SUCCESS;
}

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

static int statement_bind(ISC_STATUS_ARRAY status, zval *stmt_o, XSQLDA *sqlda, zval *b_vars)
{
    int i, array_cnt = 0, rv = SUCCESS;
    firebird_stmt *stmt = Z_STMT_P(stmt_o);

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

                    if (isc_create_blob(status, stmt->db_handle, stmt->tr_handle, &ib_blob.bl_handle, &ib_blob.bl_qd)) {
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
