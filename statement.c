#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"
#include "pdo_firebird_utils.h"

zend_class_entry *FireBird_Statement_ce;
static zend_object_handlers FireBird_Statement_object_handlers;

static void _php_firebird_free_xsqlda(XSQLDA *sqlda);

#define FETCH_ROW       1
#define FETCH_ARRAY     2

static void _php_firebird_free_stmt(firebird_stmt *s);
static void _php_firebird_fetch_hash(INTERNAL_FUNCTION_PARAMETERS, int fetch_type);

// TODO: fetch flags
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
        update_err_props(status, FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS));
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

    if (FAILURE == _php_firebird_execute(status, bind_args, num_bind_args, Z_OBJ_P(ZEND_THIS))) {
        update_err_props(status, FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

const zend_function_entry FireBird_Statement_methods[] = {
    PHP_ME(Statement, fetch_row, arginfo_FireBird_Statement_fetch_row, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, fetch_array, arginfo_FireBird_Statement_fetch_row, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, fetch_object, arginfo_FireBird_Statement_fetch_object, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, execute, arginfo_FireBird_Statement_execute, ZEND_ACC_PUBLIC)
    PHP_ME(Statement, close, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Statement_create(zend_class_entry *ce)
{
    php_printf("FireBird_Statement_create()\n");
    firebird_stmt *s = zend_object_alloc(sizeof(firebird_stmt), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Statement_free_obj(zend_object *obj)
{
    php_printf("FireBird_Statement_free_obj\n");
    firebird_stmt *s = Z_STMT_O(obj);

    _php_firebird_free_stmt(s);

    zend_object_std_dtor(&s->std);
}

void register_FireBird_Statement_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Statement", FireBird_Statement_methods);
    FireBird_Statement_ce = zend_register_internal_class(&tmp_ce);

    ADD_ERR_PROPS(FireBird_Statement_ce);

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
                update_err_props(status, FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS));
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

                        if (isc_open_blob(status, &stmt->db_handle, &stmt->tr_handle,
                            &blob_handle.bl_handle, &blob_handle.bl_qd)) {
                                update_err_props(status, FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS));
                                goto _php_firebird_fetch_error;
                        }

                        if (isc_blob_info(status, &blob_handle.bl_handle, sizeof(bl_items),
                            bl_items, sizeof(bl_info), bl_info)) {
                                update_err_props(status, FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS));
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
                            update_err_props(status, FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS));
                            goto _php_firebird_fetch_error;
                        }

                        if (isc_close_blob(status, &blob_handle.bl_handle)) {
                            update_err_props(status, FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS));
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

int _php_firebird_execute(ISC_STATUS_ARRAY status, zval *bind_args, uint32_t num_bind_args, zend_object *stmt_o)
{
    firebird_stmt *stmt = Z_STMT_O(stmt_o);

    if (num_bind_args != stmt->in_sqlda->sqld) {
        zend_throw_exception_ex(zend_ce_argument_count_error, 0,
            "Statement expects %d arguments, %d given", stmt->in_sqlda->sqld, num_bind_args);
        return FAILURE;
    }

    /* has placeholders */
    if (stmt->in_sqlda->sqld > 0) {
        if (FAILURE == _php_firebird_bind(status, stmt->in_sqlda, bind_args, stmt_o)) {
            return FAILURE;
        }
    }

    ISC_STATUS isc_result;
    if (stmt->statement_type == isc_info_sql_stmt_exec_procedure) {
        assert(false && "TODO: isc_info_sql_stmt_exec_procedure");
        // isc_result = isc_dsql_execute2(IB_STATUS, &stmt->transtmt->handle,
        //     &stmt->stmt, SQLDA_CURRENT_VERSION, in_sqlda, out_sqlda);
    } else {
        isc_result = isc_dsql_execute(status, &stmt->tr_handle, &stmt->stmt_handle, SQLDA_CURRENT_VERSION, stmt->in_sqlda);
    }

    if (isc_result) {
        return FAILURE;
    }

    stmt->has_more_rows = 1;

    return SUCCESS;
}
