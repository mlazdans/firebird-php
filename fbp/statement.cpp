#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/statement.hpp"

extern "C" {
#include "ext/date/php_date.h"
#include "firebird_utils.h"
#include "php_firebird_includes.h"
#include "statement.h"
#include "blob.h"
}

using namespace Firebird;

namespace FBP {

Statement::Statement(Transaction *tra)
    : tra{tra}
{
}

void Statement::prepare(unsigned int len_sql, const char *sql)
{
    if (statement) {
        throw Php_Firebird_Exception(zend_ce_error,
            "BUG: statement already prepared or internal structure corrupted");
    }

    IStatement *tmp = tra->prepare(len_sql, sql);

    info.statement_type = tmp->getType(&st);

    input_metadata = tmp->getInputMetadata(&st);
    info.in_vars_count = input_metadata->getCount(&st);
    in_buffer_size = input_metadata->getMessageLength(&st);
    in_buffer = (unsigned char *)calloc(in_buffer_size, sizeof(*in_buffer));

    // TODO: alloc just before fetch?
    output_metadata = tmp->getOutputMetadata(&st);
    info.out_vars_count = output_metadata->getCount(&st);
    out_buffer_size = output_metadata->getMessageLength(&st);
    out_buffer = (unsigned char *)calloc(out_buffer_size, sizeof(*out_buffer));

    // TODO: use standart alloc?
    info.sql = estrdup(sql);
    info.sql_len = len_sql;

    statement = tmp;
}

IStatement* Statement::get_statement() {
    return statement;
}

void Statement::bind(zval *b_vars, unsigned int num_bind_args)
{
    FBDEBUG("%s(in_buffer=%p)", __func__, in_buffer);
    auto util = master->getUtilInterface();

    char tmp_buf[256];
    struct tm t;
    const char *tformat;
    int64_t long_min, long_max;
    zend_long long_val;

    if (num_bind_args != info.in_vars_count) {
        slprintf(tmp_buf, sizeof(tmp_buf), "Statement expects %d arguments, %d given",
            info.in_vars_count, num_bind_args);

        throw Php_Firebird_Exception(zend_ce_argument_count_error, std::string(tmp_buf));
    }

    for (size_t i = 0; i < info.in_vars_count; ++i) {
        zval *b_var = &b_vars[i];

        auto sqlind = reinterpret_cast<short*>(in_buffer + input_metadata->getNullOffset(&st, i));
        // short *sqlind = (short*)&buffer[im->getNullOffset(&st, i)];

        if (Z_TYPE_P(b_var) == IS_NULL) {
            *sqlind = -1;
            continue;
        }

        auto sqldata = in_buffer + input_metadata->getOffset(&st, i);
        auto scale = input_metadata->getScale(&st, i);
        auto len_sqldata = input_metadata->getLength(&st, i);

        *sqlind = 0;
        // stmt->bind_buf[i].sqlind = 0;
        // sqldata = (unsigned char *)&stmt->bind_buf[i].val;

        auto sqltype = input_metadata->getType(&st, i);

        FBDEBUG("sqltype: %s, sql buflen: %d, phptype: %s",
            fbu_get_sql_type_name(sqltype), len_sqldata, zend_zval_type_name(b_var));

        // Init
        long_min = long_max = 0;
        switch (sqltype)
        {
            case SQL_SHORT:
                long_min = INT16_MIN;
                long_max = INT16_MAX;
                break;
            case SQL_LONG:
                long_min = INT32_MIN;
                long_max = INT32_MAX;
                break;
            case SQL_INT64:
                long_min = INT64_MIN;
                long_max = INT64_MAX;
                break;
        }

        // Perform PHP type conversions
        switch (sqltype)
        {
            case SQL_FLOAT:
            case SQL_DOUBLE:
                switch (Z_TYPE_P(b_var))
                {
                    case IS_DOUBLE: break;
                    case IS_STRING:
                    case IS_LONG:
                        convert_to_double(b_var);
                        break;
                    default: goto wrong_z_type;
                }
                break;

            case SQL_TYPE_DATE:
            case SQL_TIMESTAMP:
            case SQL_TIMESTAMP_TZ:
                switch (Z_TYPE_P(b_var))
                {
                    case IS_STRING: break;
                    case IS_LONG:
                        if (!php_gmtime_r(&Z_LVAL_P(b_var), &t)) {
                            slprintf(tmp_buf, sizeof(tmp_buf), "Argument %d: could not parse timestamp", i);
                            throw Php_Firebird_Exception(zend_ce_value_error, std::string(tmp_buf));
                        }
                        break;
                    default: goto wrong_z_type;
                }
                break;

            case SQL_TYPE_TIME:
            case SQL_TIME_TZ:
                if (Z_TYPE_P(b_var) != IS_STRING) goto wrong_z_type;
                break;

            case SQL_DEC16:
            case SQL_DEC34:
            case SQL_INT128:
                switch (Z_TYPE_P(b_var))
                {
                    case IS_STRING: break;
                    case IS_DOUBLE:
                    case IS_LONG:
                        convert_to_string(b_var);
                        break;
                    default: goto wrong_z_type;
                }
                break;

            case SQL_SHORT:
            case SQL_LONG:
            case SQL_INT64:
                if (!long_min || !long_max) {
                    throw Php_Firebird_Exception(zend_ce_error, "BUG: unreachable");
                }

                if (scale != 0 && Z_TYPE_P(b_var) == IS_STRING) {
                    uint64_t res;
                    int sign, exp;

                    FBDEBUG("type: %s; scale: %d; s: %s",
                        fbu_get_sql_type_name(sqltype), scale, Z_STRVAL_P(b_var)
                    );

                    int parse_err = fbu_string_to_numeric(Z_STRVAL_P(b_var), Z_STRLEN_P(b_var),
                        scale, long_max, &sign, &exp, &res);

                    if (parse_err) {
                        switch (parse_err)
                        {
                            case STRNUM_PARSE_OVERFLOW:
                                slprintf(tmp_buf, sizeof(tmp_buf),
                                    "Argument %d: %s value %zu out of range [%ld, %ld]",
                                    i, fbu_get_sql_type_name(sqltype), res, long_min, long_max
                                );
                                break;
                            default:
                                slprintf(tmp_buf, sizeof(tmp_buf), "Argument %d: string parse error", i);
                                break;
                        }
                        throw Php_Firebird_Exception(zend_ce_value_error, std::string(tmp_buf));
                    }

                    long_val = static_cast<zend_long>(sign < 0 ? -res : res);

                    FBDEBUG("sign: %d; exp: %d; res: %zu; final val: %ld",
                        sign, exp, res, long_val
                    );
                } else if (Z_TYPE_P(b_var) == IS_LONG) {
                    long_val = Z_LVAL_P(b_var);
                    if (long_val < long_min || long_val > long_max) {
                        slprintf(tmp_buf, sizeof(tmp_buf),
                            "Argument %d: %s value %ld out of range [%d, %d]",
                            i, fbu_get_sql_type_name(sqltype), long_val, long_min, long_max
                        );
                        throw Php_Firebird_Exception(zend_ce_value_error, std::string(tmp_buf));
                    }
                } else {
                    goto wrong_z_type;
                }
                break;

            case SQL_BOOLEAN: break;
            case SQL_BLOB: break;

            case SQL_TEXT:
            case SQL_VARYING:
                convert_to_string(b_var);
                if (Z_STRLEN_P(b_var) > len_sqldata) {
                    slprintf(tmp_buf, sizeof(tmp_buf),
                        "Argument %d: string of size %d does not fit buffer of size %d",
                        i, Z_STRLEN_P(b_var), len_sqldata
                    );
                    throw Php_Firebird_Exception(zend_ce_value_error, std::string(tmp_buf));
                }
                break;

            default:
                fbp_fatal("Unhandled type check for sqltype: %s(%d)", fbu_get_sql_type_name(sqltype), sqltype);
        }

        // Populate metadata buffer
        switch (sqltype)
        {
            case SQL_FLOAT:
                *reinterpret_cast<float*>(sqldata) = static_cast<float>(Z_DVAL_P(b_var));
                break;
            case SQL_DOUBLE:
                *reinterpret_cast<double*>(sqldata) = static_cast<double>(Z_DVAL_P(b_var));
                break;
            case SQL_TYPE_DATE:
                switch (Z_TYPE_P(b_var))
                {
                    case IS_LONG:
                        isc_encode_sql_date(&t, reinterpret_cast<ISC_DATE*>(sqldata));
                        break;
                    case IS_STRING:
                        tformat = "Y-m-d";
                        goto parse_datetime;
                    default:
                        throw Php_Firebird_Exception(zend_ce_error, "BUG: unreachable");
                }
                break;
            case SQL_TIMESTAMP_TZ:
                switch (Z_TYPE_P(b_var))
                {
                    case IS_LONG:
                    {
                        ISC_TIMESTAMP_TZ *ts = reinterpret_cast<ISC_TIMESTAMP_TZ*>(sqldata);
                        isc_encode_sql_date(&t, &ts->utc_timestamp.timestamp_date);
                        isc_encode_sql_time(&t, &ts->utc_timestamp.timestamp_time);
                        ts->time_zone = 0;
                    } break;
                    case IS_STRING:
                        tformat = "Y-m-d H:i:s e";
                        goto parse_datetime;
                    default:
                        throw Php_Firebird_Exception(zend_ce_error, "BUG: unreachable");
                }
                break;
            case SQL_TIMESTAMP:
                switch (Z_TYPE_P(b_var))
                {
                    case IS_LONG:
                    {
                        ISC_TIMESTAMP *ts = reinterpret_cast<ISC_TIMESTAMP*>(sqldata);
                        isc_encode_sql_date(&t, &ts->timestamp_date);
                        isc_encode_sql_time(&t, &ts->timestamp_time);
                    } break;
                    case IS_STRING:
                        tformat = "Y-m-d H:i:s";
                        goto parse_datetime;
                    default:
                        throw Php_Firebird_Exception(zend_ce_error, "BUG: unreachable");
                }
                break;
            case SQL_TYPE_TIME:
                tformat = "H:i:s";
                goto parse_datetime;
            case SQL_TIME_TZ:
                tformat = "H:i:s e";
                goto parse_datetime;
            case SQL_SHORT:
                *reinterpret_cast<int16_t*>(sqldata) = static_cast<int16_t>(long_val);
                break;
            case SQL_LONG:
                *reinterpret_cast<int32_t*>(sqldata) = static_cast<int32_t>(long_val);
                break;
            case SQL_INT64:
                *reinterpret_cast<int64_t*>(sqldata) = static_cast<int64_t>(long_val);
                break;
            case SQL_INT128:
                util->getInt128(&st)->fromString(&st, scale, Z_STRVAL_P(b_var), reinterpret_cast<FB_I128*>(sqldata));
                break;
            case SQL_DEC16:
                util->getDecFloat16(&st)->fromString(&st, Z_STRVAL_P(b_var), reinterpret_cast<FB_DEC16*>(sqldata));
                break;
            case SQL_DEC34:
                util->getDecFloat34(&st)->fromString(&st, Z_STRVAL_P(b_var), reinterpret_cast<FB_DEC34*>(sqldata));
                break;

            case SQL_BLOB:
                ISC_QUAD bl_id;
                if (Z_TYPE_P(b_var) == IS_OBJECT && Z_OBJCE_P(b_var) == FireBird_Blob_Id_ce) {
                    bl_id = get_firebird_blob_id_from_zval(b_var)->bl_id;
                } else if (Z_TYPE_P(b_var) == IS_OBJECT && Z_OBJCE_P(b_var) == FireBird_Blob_ce) {
                    bl_id = get_firebird_blob_from_zval(b_var)->info->id;
                } else {
                    convert_to_string(b_var);
                    bl_id = tra->create_blob(Z_STR_P(b_var));
                }
                *reinterpret_cast<ISC_QUAD*>(sqldata) = static_cast<ISC_QUAD>(bl_id);
                break;

            case SQL_BOOLEAN:
                if (Z_TYPE_P(b_var) == IS_FALSE) {
                    *reinterpret_cast<FB_BOOLEAN*>(sqldata) = FB_FALSE;
                } else if (Z_TYPE_P(b_var) == IS_TRUE) {
                    *reinterpret_cast<FB_BOOLEAN*>(sqldata) = FB_TRUE;
                } else {
                    goto wrong_z_type;
                }
                break;

            case SQL_ARRAY:
                fbp_fatal("ARRAY type is not supported.");

            case SQL_VARYING: {
                auto buf = reinterpret_cast<firebird_vary*>(sqldata);
                buf->vary_length = Z_STRLEN_P(b_var);
                memcpy(buf->vary_string, Z_STRVAL_P(b_var), buf->vary_length);
                // Z_STRVAL_P(b_var)[buf->vary_length] = '\0';
            } break;

            case SQL_TEXT:
                memcpy(sqldata, Z_STRVAL_P(b_var), Z_STRLEN_P(b_var));
                memset(&sqldata[Z_STRLEN_P(b_var)], ' ', len_sqldata - Z_STRLEN_P(b_var));
                break;

            default:
                fbp_fatal("Unhandled sqltype: %d", sqltype);

        } /* switch */

        continue;

wrong_z_type: {
        slprintf(tmp_buf, sizeof(tmp_buf),
            "Argument %d: type mismatch '%s' to SQL type '%s'", i,
            zend_get_type_by_const(Z_TYPE_P(b_var)), fbu_get_sql_type_name(sqltype));

        throw Php_Firebird_Exception(zend_ce_type_error, std::string(tmp_buf));
    }

parse_datetime: {
        zval dateo;
        php_date_obj *obj;

        php_date_instantiate(php_date_get_date_ce(), &dateo);
        obj = Z_PHPDATE_P(&dateo);

        if (!php_date_initialize(obj, Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), tformat, NULL, PHP_DATE_INIT_FORMAT)) {
            slprintf(tmp_buf, sizeof(tmp_buf),
                "Argument %d: parse date failed, "
                "call date_get_last_errors() for more information",
                i
            );

            zval_ptr_dtor(&dateo);

            throw Php_Firebird_Exception(zend_ce_type_error, std::string(tmp_buf));
        }

        if (sqltype == SQL_TIME_TZ) {
            util->encodeTimeTz(&st, reinterpret_cast<ISC_TIME_TZ*>(sqldata),
                obj->time->h, obj->time->i, obj->time->s, obj->time->us, obj->time->tz_info->name
            );
        } else if (sqltype == SQL_TYPE_TIME) {
            *reinterpret_cast<ISC_TIME*>(sqldata) = util->encodeTime(
                obj->time->h, obj->time->i, obj->time->s, obj->time->us
            );
        } else if (sqltype == SQL_TYPE_DATE) {
            *reinterpret_cast<ISC_DATE*>(sqldata) = util->encodeDate(
                obj->time->y, obj->time->m, obj->time->d
            );
        } else if (sqltype == SQL_TIMESTAMP) {
            ISC_TIMESTAMP *ts = reinterpret_cast<ISC_TIMESTAMP*>(sqldata);
            ts->timestamp_date = util->encodeDate(obj->time->y, obj->time->m, obj->time->d);
            ts->timestamp_time = util->encodeTime(obj->time->h, obj->time->i, obj->time->s, obj->time->us);
        } else if (sqltype == SQL_TIMESTAMP_TZ) {
            util->encodeTimeStampTz(&st, reinterpret_cast<ISC_TIMESTAMP_TZ*>(sqldata),
                obj->time->y, obj->time->m, obj->time->d,
                obj->time->h, obj->time->i, obj->time->s, obj->time->us,
                obj->time->tz_info->name
            );
        } else {
            throw Php_Firebird_Exception(zend_ce_error, "BUG: unreachable");
        }

        zval_ptr_dtor(&dateo);
    } // parse_datetime:
    } // for
} // bind

void Statement::open_cursor()
{
    // TODO: check if curs already opened
    cursor = tra->open_cursor(statement, input_metadata, in_buffer, output_metadata);
}

int Statement::close_cursor()
{
    if (cursor) {
        cursor->close(&st);
        cursor->release();
        cursor = nullptr;
        return SUCCESS;
    } else {
        return FAILURE;
    }
}

int Statement::fetch_next()
{
    FBDEBUG("Statement::fetch_next(this=%p, out_buffer=%p)", this, out_buffer);
    auto fetch_res = cursor->fetchNext(&st, out_buffer);
    if (fetch_res == IStatus::RESULT_NO_DATA) {
        cursor->close(&st);
        cursor->release();
        cursor = nullptr;
    }
    return fetch_res;
}

HashTable *Statement::output_buffer_to_array(int flags)
{
    HashTable *hash;
    zval *result;

    if (flags & FBP_FETCH_HASHED) {
        if (!ht_aliases) alloc_ht_aliases();
        hash = zend_array_dup(ht_aliases);
    } else if (flags & FBP_FETCH_INDEXED) {
        if (!ht_ind) alloc_ht_ind();
        hash = zend_array_dup(ht_ind);
    } else {
        throw Php_Firebird_Exception(zend_ce_error, "BUG: fetch method not set");
    }

    for (unsigned int index = 0; index < info.out_vars_count; index++) {
        result = zend_hash_get_current_data(hash);
        var_zval(result, index, flags);
        zend_hash_move_forward(hash);
    }

    zend_hash_internal_pointer_reset(hash);

    return hash;
}

int Statement::var_zval(zval *val, unsigned int index, int flags)
{
    FBDEBUG("%s(om=%p, out_buffer=%p)", __func__, output_metadata, out_buffer);
    IUtil* util = master->getUtilInterface();

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

    auto nullind = *(int16_t*)(out_buffer + output_metadata->getNullOffset(&st, index));

    if (nullind) {
        ZVAL_NULL(val);
        return SUCCESS;
    }

    auto type = output_metadata->getType(&st, index);
    auto len = output_metadata->getLength(&st, index);
    auto scale = output_metadata->getScale(&st, index);
    auto data = out_buffer + output_metadata->getOffset(&st, index);

    FBDEBUG("   alias=%s, type: %s", output_metadata->getAlias(&st, index),
        fbu_get_sql_type_name(type));

    char str_buf[255];
    size_t str_len;

    zend_long long_val;

    const char *t_format;
    unsigned t_year, t_month, t_day, t_hours, t_minutes, t_seconds, t_fractions;
    char t_zone_buff[40];
    zval t_dateo;
    php_date_obj *t_date_obj;

    switch (type)
    {
        default:
            fbp_fatal("unhandled type: %d, field: %s", type, output_metadata->getAlias(&st, index));

        case SQL_VARYING:
            len = ((firebird_vary *) data)->vary_length;
            data = ((firebird_vary *) data)->vary_string;
            /* no break */
        case SQL_TEXT:
            ZVAL_STRINGL(val, (char*)data, len);
            break;
        case SQL_BOOLEAN:
            ZVAL_BOOL(val, *(FB_BOOLEAN *) data);
            break;
        case SQL_SHORT:
            long_val = *(int16_t*) data;
            goto _sql_long;
        case SQL_LONG:
            long_val = *(int32_t*) data;
            goto _sql_long;
        case SQL_INT128:
            util->getInt128(&st)->toString(&st, (FB_I128 *)data, scale, sizeof(str_buf), str_buf);
            ZVAL_STRING(val, str_buf);
            break;
        case SQL_DEC16:
            util->getDecFloat16(&st)->toString(&st, (FB_DEC16 *)data, sizeof(str_buf), str_buf);
            ZVAL_STRING(val, str_buf);
            break;
        case SQL_DEC34:
            util->getDecFloat34(&st)->toString(&st, (FB_DEC34 *)data, sizeof(str_buf), str_buf);
            ZVAL_STRING(val, str_buf);
            break;
        case SQL_TYPE_DATE:
            t_format = "Y-m-d";
            util->decodeDate(*(ISC_DATE *)data, &t_year, &t_month, &t_day);
            t_hours = t_minutes = t_seconds = t_fractions = 0;
            fbu_init_date_object(NULL, &t_dateo);
            goto _set_datetime;
        case SQL_TYPE_TIME:
            t_format = "H:i:s";
            util->decodeTime(*(ISC_TIME *)data, &t_hours, &t_minutes, &t_seconds, &t_fractions);
            t_year = t_month = t_day = 0;
            fbu_init_date_object(NULL, &t_dateo);
            goto _set_datetime;
        case SQL_TIME_TZ:
            t_format = "H:i:s e";
            util->decodeTimeTz(&st, (ISC_TIME_TZ *)data, &t_hours, &t_minutes, &t_seconds, &t_fractions,
                sizeof(t_zone_buff), t_zone_buff);
            t_year = t_month = t_day = 0;
            fbu_init_date_object(t_zone_buff, &t_dateo);
            goto _set_datetime;
        case SQL_TIMESTAMP_TZ:
            t_format = "Y-m-d H:i:s e";
            util->decodeTimeStampTz(&st, (ISC_TIMESTAMP_TZ *)data, &t_year, &t_month, &t_day, &t_hours,
                &t_minutes, &t_seconds, &t_fractions, sizeof(t_zone_buff), t_zone_buff);
            fbu_init_date_object(t_zone_buff, &t_dateo);
            goto _set_datetime;
        case SQL_TIMESTAMP:
            {
                t_format = "Y-m-d H:i:s";
                ISC_TIMESTAMP *ts = (ISC_TIMESTAMP *) data;
                util->decodeDate(ts->timestamp_date, &t_year, &t_month, &t_day);
                util->decodeTime(ts->timestamp_time, &t_hours, &t_minutes, &t_seconds, &t_fractions);
                fbu_init_date_object(NULL, &t_dateo);
            }

_set_datetime:
            t_date_obj = Z_PHPDATE_P(&t_dateo);
            t_date_obj->time->y = t_year;
            t_date_obj->time->m = t_month;
            t_date_obj->time->d = t_day;
            t_date_obj->time->h = t_hours;
            t_date_obj->time->i = t_minutes;
            t_date_obj->time->s = t_seconds;
            t_date_obj->time->us = t_fractions;

            // efree(date_obj->time->tz_abbr);
            // efree(date_obj->time);

            if (flags & FBP_FETCH_DATE_OBJ) {
                ZVAL_COPY(val, &t_dateo);
            } else {
                ZVAL_STR(val, php_format_date_obj(t_format, strlen(t_format), t_date_obj));
            }

            zval_ptr_dtor(&t_dateo);
            break;

        case SQL_BLOB:
            if (flags & FBP_FETCH_BLOB_TEXT) {
                ZVAL_STR(val, tra->get_blob_contents((ISC_QUAD *)data));
            } else {
                object_init_ex(val, FireBird_Blob_Id_ce);
                FireBird_Blob_Id___construct(val, *(ISC_QUAD *)data);
            }
            break;

#if 0
#if (SIZEOF_ZEND_LONG >= 8)
            goto _sql_long;
#else
            // TODO: 32-bit handling
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
#endif
        case SQL_FLOAT:
            ZVAL_DOUBLE(val, *(float *) data);
            break;
        case SQL_DOUBLE:
            ZVAL_DOUBLE(val, *(double *) data);
            break;
        case SQL_INT64:
            long_val = *(int64_t*) data;
_sql_long:
            if (scale == 0) {
                ZVAL_LONG(val, long_val);
            } else {
                zend_long f = (zend_long) scales[-scale];

                if (long_val >= 0) {
                    str_len = slprintf(str_buf, sizeof(str_buf), ZEND_LONG_FMT ".%0*" ZEND_LONG_FMT_SPEC, long_val / f, -scale,  long_val % f);
                } else if (long_val <= -f) {
                    str_len = slprintf(str_buf, sizeof(str_buf), ZEND_LONG_FMT ".%0*" ZEND_LONG_FMT_SPEC, long_val / f, -scale,  -long_val % f);
                } else {
                    str_len = slprintf(str_buf, sizeof(str_buf), "-0.%0*" ZEND_LONG_FMT_SPEC, -scale, -long_val % f);
                }
                ZVAL_STRINGL(val, str_buf, str_len);
            }
            break;
    } /* switch (type) */

    return SUCCESS;
}

void Statement::execute()
{
    tra->execute_statement(statement, input_metadata, in_buffer, output_metadata, out_buffer);
    query_statistics();
}

Statement::~Statement() noexcept
{
    FBDEBUG("~Statement(this=%p)", this);

    int err = 0;
    try
    {
        if (cursor) cursor->close(&st);
        if (statement) statement->free(&st);
    }
    catch (...)
    {
        err = 1;
    }

    if (cursor) {
        cursor->release();
        cursor = nullptr;
    }

    if (statement) {
        statement->release();
        statement = nullptr;
    }

    if (input_metadata) {
        input_metadata->release();
        input_metadata = nullptr;
    }

    if (output_metadata) {
        output_metadata->release();
        output_metadata = nullptr;
    }

    if (in_buffer) {
        free(in_buffer);
        in_buffer = nullptr;
    }

    if (out_buffer) {
        free(out_buffer);
        out_buffer = nullptr;
    }

    if (ht_aliases) {
        zend_array_destroy(ht_aliases);
        ht_aliases = nullptr;
    }

    if (ht_ind) {
        zend_array_destroy(ht_ind);
        ht_ind = nullptr;
    }

    if (info.sql) {
        efree((void *)info.sql);
        info.sql = nullptr;
    }

    if (err) fbu_handle_exception2();
}

void Statement::insert_alias(const char *alias)
{
    char buf[METADATALENGTH + 3 + 1]; // _00 + \0
    zval t2;
    int i = 0;
    char const *base = "FIELD"; /* use 'FIELD' if name is empty */

    size_t alias_len = strlen(alias);
    size_t alias_len_w_suff = alias_len + 3;

    switch (*alias) {
        void *p;

        default:
            i = 1;
            base = alias;

            while ((p = zend_symtable_str_find_ptr(ht_aliases, alias, alias_len)) != NULL) {
        case '\0':
                // TODO: i > 99?
                snprintf(buf, sizeof(buf), "%s_%02d", base, i++);
                alias = buf;
                alias_len = alias_len_w_suff;
            }
    }

    ZVAL_NULL(&t2);
    zend_hash_str_add_new(ht_aliases, alias, alias_len, &t2);
}

void Statement::alloc_ht_aliases()
{
    ALLOC_HASHTABLE(ht_aliases);
    zend_hash_init(ht_aliases, info.out_vars_count, NULL, ZVAL_PTR_DTOR, 0);

    for (unsigned int index = 0; index < info.out_vars_count; index++) {
        insert_alias(output_metadata->getAlias(&st, index));
    }
}

void Statement::alloc_ht_ind()
{
    ALLOC_HASHTABLE(ht_ind);
    zend_hash_init(ht_ind, info.out_vars_count, NULL, ZVAL_PTR_DTOR, 0);

    zval t2;
    ZVAL_NULL(&t2);

    for (unsigned int index = 0; index < info.out_vars_count; index++) {
        zend_hash_index_add(ht_ind, index, &t2);
    }
}

void Statement::query_statistics()
{
    auto util = master->getUtilInterface();

    unsigned char req[] = { isc_info_sql_records };
    unsigned char resp[64] = { 0 };

    info.insert_count = 0;
    info.update_count = 0;
    info.delete_count = 0;
    info.select_count = 0;
    info.affected_count = 0;

    statement->getInfo(&st, sizeof(req), req, sizeof(resp), resp);

    const ISC_UCHAR* p = resp + 3;

    if (resp[0] != isc_info_sql_records)
    {
        fbp_fatal("Unexpected info response tag: %d", resp[0]);
    }

    while ((*p != isc_info_end) && (p - (const ISC_UCHAR *)&resp < sizeof(resp)))
    {
        const ISC_UCHAR count_is = *p++;
        const ISC_SHORT len = isc_portable_integer(p, 2); p += 2;
        const ISC_ULONG count = isc_portable_integer(p, len); p += len;
        switch(count_is) {
            case isc_info_req_insert_count: info.insert_count += count; break;
            case isc_info_req_update_count: info.update_count += count; break;
            case isc_info_req_delete_count: info.delete_count += count; break;
            case isc_info_req_select_count: info.select_count += count; break;
            default:
                fbp_fatal("BUG: unrecognized isc_dsql_sql_info item: %d with value: %d", count_is, count);
                break;
        }
    }

    FBDEBUG("%s\n  insert_count=%d, update_count=%d, delete_count=%d, select_count=%d",
        __func__, info.insert_count, info.update_count, info.delete_count, info.select_count);

    info.affected_count = info.insert_count + info.update_count + info.delete_count;
}

firebird_stmt_info *Statement::get_info()
{
    return &info;
}

} // namespace
