#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ibase.h>
#include <firebird/fb_c_api.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "ext/spl/spl_exceptions.h"

#include "php_firebird_includes.h"
#include "pdo_firebird_utils.h"

#include "statement.h"
#include "transaction.h"
#include "blob.h"

zend_class_entry *FireBird_Statement_ce;
static zend_object_handlers FireBird_Statement_object_handlers;

#define FETCH_ROW       1
#define FETCH_ARRAY     2

void FireBird_Statement___construct(zval *Stmt, zval *Transaction)
{
    firebird_stmt *stmt = get_firebird_stmt_from_zval(Stmt);
    firebird_trans *tr = get_firebird_trans_from_zval(Transaction);

    fbp_statement_ctor(stmt, tr);

    OBJ_SET(FireBird_Statement_ce, Stmt, "transaction", Transaction);
}

PHP_METHOD(Statement, __construct)
{
    zval *Transaction;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Transaction, FireBird_Transaction_ce)
    ZEND_PARSE_PARAMETERS_END();

    FireBird_Statement___construct(ZEND_THIS, Transaction);
}

static void _FireBird_Statement_fetch(INTERNAL_FUNCTION_PARAMETERS, int fetch_type)
{
    zend_long flags = 0;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();

    FireBird_Statement_fetch(ZEND_THIS, return_value, flags, fetch_type);
}

PHP_METHOD(Statement, fetch_row)
{
    _FireBird_Statement_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, FETCH_ROW);
}

PHP_METHOD(Statement, fetch_array)
{
    _FireBird_Statement_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, FETCH_ARRAY);
}

PHP_METHOD(Statement, fetch_object)
{
    _FireBird_Statement_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, FETCH_ARRAY);

    if (Z_TYPE_P(return_value) == IS_ARRAY) {
        convert_to_object(return_value);
    }
}

PHP_METHOD(Statement, close)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);

    if (isc_dsql_free_statement(FBG(status), &stmt->stmt_handle, DSQL_close)) {
        update_err_props(FBG(status), FireBird_Statement_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(Statement, free)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);

    if (isc_dsql_free_statement(FBG(status), &stmt->stmt_handle, DSQL_drop)) {
        update_err_props(FBG(status), FireBird_Statement_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    stmt->stmt_handle = 0;

    RETURN_TRUE;
}

int FireBird_Statement_execute(zval *Stmt, zval *bind_args, uint32_t num_bind_args)
{
    firebird_stmt *stmt = get_firebird_stmt_from_zval(Stmt);

    if (!stmt->statement_type) {
        zend_throw_exception_ex(spl_ce_RuntimeException, 0, "Execute with unprepared statement.");
        return FAILURE;
    }

    if (num_bind_args != stmt->in_sqlda->sqld) {
        zend_throw_exception_ex(zend_ce_argument_count_error, 0,
            "Statement expects %d arguments, %d given", stmt->in_sqlda->sqld, num_bind_args);
        return FAILURE;
    }

    if (fbp_statement_execute(stmt, bind_args, num_bind_args)) {
        update_err_props(FBG(status), FireBird_Statement_ce, Stmt);
        return FAILURE;
    }

    zend_update_property_long(FireBird_Statement_ce, Z_OBJ_P(Stmt), "insert_count", sizeof("insert_count") - 1, stmt->insert_count);
    zend_update_property_long(FireBird_Statement_ce, Z_OBJ_P(Stmt), "update_count", sizeof("update_count") - 1, stmt->update_count);
    zend_update_property_long(FireBird_Statement_ce, Z_OBJ_P(Stmt), "delete_count", sizeof("delete_count") - 1, stmt->delete_count);
    zend_update_property_long(FireBird_Statement_ce, Z_OBJ_P(Stmt), "affected_count", sizeof("affected_count") - 1, stmt->affected_count);

    return SUCCESS;
}

PHP_METHOD(Statement, execute)
{
    zval *bind_args;
    uint32_t num_bind_args;

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    RETVAL_BOOL(SUCCESS == FireBird_Statement_execute(ZEND_THIS, bind_args, num_bind_args));
}

int FireBird_Statement_prepare(zval *Stmt, const ISC_SCHAR* sql)
{
    firebird_stmt *stmt = get_firebird_stmt_from_zval(Stmt);

    if (fbp_statement_prepare(stmt, sql)) {
        ISC_INT64 error_code_long = update_err_props(FBG(status), FireBird_Statement_ce, Stmt);

        // Do we CREATE DATABASE?
        if (error_code_long == isc_dsql_crdb_prepare_err) {
            fbp_error("CREATE DATABASE detected on active connection. Use Database::create() instead.");
        }
        return FAILURE;
    }

    zend_update_property_long(FireBird_Statement_ce, Z_OBJ_P(Stmt), "num_vars_in", sizeof("num_vars_in") - 1,
        stmt->in_sqlda->sqld
    );

    zend_update_property_long(FireBird_Statement_ce, Z_OBJ_P(Stmt), "num_vars_out", sizeof("num_vars_out") - 1,
        stmt->out_sqlda->sqld
    );

    return SUCCESS;
}

PHP_METHOD(Statement, prepare)
{
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    RETVAL_BOOL(SUCCESS == FireBird_Statement_prepare(ZEND_THIS, ZSTR_VAL(sql)));
}

int FireBird_Statement_query(zval *Stmt, const ISC_SCHAR* sql, zval *bind_args, uint32_t num_bind_args)
{
    if (FireBird_Statement_prepare(Stmt, sql)) {
        return FAILURE;
    }

    if (FireBird_Statement_execute(Stmt, bind_args, num_bind_args)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(Statement, query)
{
    zval *bind_args;
    uint32_t num_bind_args;
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(sql)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    RETVAL_BOOL(SUCCESS == FireBird_Statement_query(ZEND_THIS, ZSTR_VAL(sql), bind_args, num_bind_args));
}

void statement_var_info(zval *return_value, XSQLVAR *var)
{
    object_init_ex(return_value, FireBird_Var_Info_ce);
    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "name", sizeof("name") - 1, var->sqlname);
    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "alias", sizeof("alias") - 1, var->aliasname);
    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "relation", sizeof("relation") - 1, var->relname);
    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "byte_length", sizeof("byte_length") - 1, var->sqllen);
    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "type", sizeof("type") - 1, var->sqltype & ~1);
    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "sub_type", sizeof("sub_type") - 1, var->sqlsubtype);
    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "scale", sizeof("scale") - 1, var->sqlscale);
    zend_update_property_bool(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "nullable", sizeof("nullable") - 1, var->sqltype & 1);
}

PHP_METHOD(Statement, get_var_info_in)
{
    zend_long num;
    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(num)
    ZEND_PARSE_PARAMETERS_END();

    if (num < 0 || num >= stmt->in_sqlda->sqld) {
        RETURN_FALSE;
    }

    statement_var_info(return_value, stmt->in_sqlda->sqlvar + num);
}

PHP_METHOD(Statement, get_var_info_out)
{
    zend_long num;
    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(num)
    ZEND_PARSE_PARAMETERS_END();

    if (num < 0 || num >= stmt->out_sqlda->sqld) {
        RETURN_FALSE;
    }

    statement_var_info(return_value, stmt->out_sqlda->sqlvar + num);
}

PHP_METHOD(Statement, set_name)
{
    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);
    char *name;
    size_t name_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(name, name_len)
    ZEND_PARSE_PARAMETERS_END();

    if (isc_dsql_set_cursor_name(FBG(status), &stmt->stmt_handle, name, 0)) {
        update_err_props(FBG(status), FireBird_Statement_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    stmt->name = name;

    zend_update_property_string(FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS), "name", sizeof("name") - 1, name);

    RETURN_TRUE;
}

const zend_function_entry FireBird_Statement_methods[] = {
    PHP_ME(Statement, __construct, arginfo_FireBird_Statement___construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Statement, fetch_row, arginfo_FireBird_Statement_fetch_row, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, fetch_array, arginfo_FireBird_Statement_fetch_row, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, fetch_object, arginfo_FireBird_Statement_fetch_object, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, prepare, arginfo_FireBird_Statement_prepare, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, execute, arginfo_FireBird_Statement_execute, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, query, arginfo_FireBird_Statement_query, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, close, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, free, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, get_var_info_in, arginfo_FireBird_Statement_get_var_info_in_out, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, get_var_info_out, arginfo_FireBird_Statement_get_var_info_in_out, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, set_name, arginfo_FireBird_Statement_set_name, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *new_FireBird_Statement(zend_class_entry *ce)
{
    FBDEBUG("new_FireBird_Statement()");

    firebird_stmt *s = zend_object_alloc(sizeof(firebird_stmt), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void free_FireBird_Statement(zend_object *obj)
{
    FBDEBUG("free_FireBird_Statement");

    firebird_stmt *stmt = get_firebird_stmt_from_obj(obj);

    if (stmt->stmt_handle) {
        if (isc_dsql_free_statement(FBG(status), &stmt->stmt_handle, DSQL_drop)) {
            status_fbp_error(FBG(status));
        } else {
            stmt->stmt_handle = 0;
        }
    }

    fbp_statement_free(stmt);

    zend_object_std_dtor(&stmt->std);
}

void register_FireBird_Statement_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Statement", FireBird_Statement_methods);
    FireBird_Statement_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Statement_ce, transaction, FireBird\\Transaction, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_STRING(FireBird_Statement_ce, name, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Statement_ce, num_vars_in, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Statement_ce, num_vars_out, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Statement_ce, insert_count, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Statement_ce, update_count, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Statement_ce, delete_count, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Statement_ce, affected_count, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Statement_ce);

    zend_class_implements(FireBird_Statement_ce, 1, FireBird_IError_ce);

    FireBird_Statement_ce->create_object = new_FireBird_Statement;
    FireBird_Statement_ce->default_object_handlers = &FireBird_Statement_object_handlers;

    memcpy(&FireBird_Statement_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Statement_object_handlers.offset = XtOffsetOf(firebird_stmt, std);
    FireBird_Statement_object_handlers.free_obj = free_FireBird_Statement;
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

            if (flags & FBP_FETCH_UNIXTIME) {
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
            if (flags & FBP_FETCH_UNIXTIME) {
                ZVAL_LONG(val, mktime(&t));
            } else {
                l = strftime(string_data, sizeof(string_data), format, &t);
                ZVAL_STRINGL(val, string_data, l);
                break;
            }
    } /* switch (type) */
    return SUCCESS;
}

void FireBird_Statement_fetch(zval *Stmt, zval *return_value, int flags, int fetch_type)
{
    zval *result_arg;
    zend_long i;
    firebird_stmt *stmt = get_firebird_stmt_from_zval(Stmt);

    if (stmt->out_sqlda == NULL || !stmt->has_more_rows) {
        RETURN_NULL();
    }

    // exec_procedure has no cursor
    if (stmt->statement_type != isc_info_sql_stmt_exec_procedure) {
        ISC_STATUS result = isc_dsql_fetch(FBG(status), &stmt->stmt_handle, 1, stmt->out_sqlda);
        if (result) {
            if (result == 100L) {
                stmt->has_more_rows = 0;
                RETURN_NULL();
            } else {
                update_err_props(FBG(status), FireBird_Statement_ce, Stmt);
                RETURN_FALSE;
            }
        }
    } else {
        stmt->has_more_rows = 0;
    }

    array_init(return_value);

    // TODO: this behaviour should be configurable: add suffix, replace, ignore
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
                    if (flags & FBP_FETCH_BLOBS) {
                        firebird_blob blob = {0};

                        fbp_blob_ctor(&blob, stmt->db_handle, stmt->tr_handle);
                        blob.bl_id = *(ISC_QUAD *) var->sqldata;

                        if (fbp_blob_open(&blob) || fbp_blob_get(&blob, &result, 0) || fbp_blob_close(&blob)) {
                            update_err_props(FBG(status), FireBird_Statement_ce, Stmt);
                            goto _php_firebird_fetch_error;
                        }
                    } else {
                        object_init_ex(&result, FireBird_Blob_Id_ce);
                        FireBird_Blob_Id___construct(&result, *(ISC_QUAD *) var->sqldata);
                    }
                    break;
                case SQL_ARRAY:
                    fbp_fatal("ARRAY type is not supported.");
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
