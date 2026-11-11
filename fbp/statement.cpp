#include <cfloat>
#include <cstdint>
#include <string>
#include <firebird/Interface.h>

#include "./base.hpp"
#include "./database.hpp"
#include "./statement.hpp"
#include "../firebird_php.hpp"

extern "C" {
#include "php_globals.h"
#include "zend_smart_str.h"
#include "ext/date/php_date.h"
#include "../firebird_utils.h"
}

#ifdef _MSC_VER
#define timegm _mkgmtime
#endif

using namespace Firebird;

namespace FBP {

Statement::Statement(Transaction &tra)
    : tra{tra}
{
    FBDEBUG("new Statement(tra=%p)=%p", PTR(tra), PTR(*this));
}

void Statement::prepare(unsigned int len_sql, const char *sql)
{
    if (stmt) {
        throw Php_Firebird_Exception(zend_ce_error,
            "BUG: statement already prepared or internal structure corrupted");
    }

    // Owned from here on: if anything below throws, ~Statement frees it
    stmt = tra.prepare_statement(len_sql, sql);
    IStatement *tmp = stmt;

    info.statement_type = tmp->getType(&st);

    input_metadata = tmp->getInputMetadata(&st);
    info.in_vars_count = input_metadata->getCount(&st);
    in_buffer_size = input_metadata->getMessageLength(&st);
    in_buffer = static_cast<unsigned char *>(calloc(in_buffer_size ? in_buffer_size : 1, 1));

    output_metadata = tmp->getOutputMetadata(&st);
    info.out_vars_count = output_metadata->getCount(&st);
    out_buffer_size = output_metadata->getMessageLength(&st);
    out_buffer = static_cast<unsigned char *>(calloc(out_buffer_size ? out_buffer_size : 1, 1));

    if (!in_buffer || !out_buffer) throw std::bad_alloc();

    info.sql = estrndup(sql, len_sql);
    info.sql_len = len_sql;

    in_vars.resize(info.in_vars_count);
    out_vars.resize(info.out_vars_count);

    for (unsigned int index = 0; index < info.in_vars_count; index++) {
        in_vars[index] = firebird_var {
            input_metadata->getField(&st, index),
            input_metadata->getAlias(&st, index),
            input_metadata->getRelation(&st, index),
            input_metadata->getType(&st, index),
            input_metadata->getSubType(&st, index),
            input_metadata->getLength(&st, index),
            input_metadata->getScale(&st, index),
            (int)input_metadata->isNullable(&st, index),
            input_metadata->getOffset(&st, index),
            input_metadata->getNullOffset(&st, index),
        };
    }

    for (unsigned int index = 0; index < info.out_vars_count; index++) {
        out_vars[index] = firebird_var {
            output_metadata->getField(&st, index),
            output_metadata->getAlias(&st, index),
            output_metadata->getRelation(&st, index),
            output_metadata->getType(&st, index),
            output_metadata->getSubType(&st, index),
            output_metadata->getLength(&st, index),
            output_metadata->getScale(&st, index),
            (int)output_metadata->isNullable(&st, index),
            output_metadata->getOffset(&st, index),
            output_metadata->getNullOffset(&st, index),
        };
    }
}

namespace {

template <typename... Args>
std::string fmt(const char *format, Args... args)
{
    char buf[512];
    snprintf(buf, sizeof(buf), format, args...);
    return std::string(buf);
}

// Holds a temporary string produced while converting one bind argument.
struct StrGuard {
    zend_string *s = nullptr;
    ~StrGuard() { if (s) zend_string_release(s); }
};

// Doubles are formatted with serialize_precision (default -1: the shortest text that
// reads back as the identical double), like var_export() and json_encode(). The
// `precision` ini used by string casts keeps only 14 digits.
zend_string *double_to_string(double d)
{
    smart_str str = {0};
    smart_str_append_double(&str, d, (int)PG(serialize_precision), false);
    return smart_str_extract(&str);
}

bool is_stringable(const zval *v)
{
    switch (Z_TYPE_P(v)) {
        case IS_STRING: case IS_LONG: case IS_DOUBLE: case IS_TRUE: case IS_FALSE:
            return true;
        case IS_OBJECT:
            return Z_OBJCE_P(v)->__tostring != nullptr;
        default:
            return false;
    }
}

// Returns the string form of a scalar or Stringable. Only call after is_stringable().
const zend_string *as_string(zval *v, StrGuard &guard)
{
    if (Z_TYPE_P(v) == IS_STRING) return Z_STR_P(v);
    guard.s = Z_TYPE_P(v) == IS_DOUBLE ? double_to_string(Z_DVAL_P(v)) : zval_get_string(v);
    return guard.s;
}

bool scale_int64(int64_t v, int scale, int64_t *out)
{
    for (int k = scale; k < 0; k++) {
        if (v > INT64_MAX / 10 || v < INT64_MIN / 10) return false;
        v *= 10;
    }
    *out = v;
    return true;
}

// Firebird zone name for a timelib time: region id, or "+HH:MM" for offsets/abbreviations
const char *zone_name(const timelib_time *t, char *buf, size_t buf_size)
{
    timelib_sll offset;

    switch (t->zone_type) {
        case TIMELIB_ZONETYPE_ID:
            if (t->tz_info) return t->tz_info->name;
            offset = 0;
            break;
        case TIMELIB_ZONETYPE_OFFSET:
            offset = t->z;
            break;
        case TIMELIB_ZONETYPE_ABBR:
            offset = t->z + t->dst * 3600;
            break;
        default:
            offset = 0;
    }

    timelib_sll a = offset < 0 ? -offset : offset;
    snprintf(buf, buf_size, "%c%02d:%02d", offset < 0 ? '-' : '+', (int)(a / 3600), (int)((a % 3600) / 60));

    return buf;
}

} // anonymous namespace

void Statement::bind(zval *b_vars, unsigned int num_bind_args)
{
    FBDEBUG("%s(in_buffer=%p)", __func__, in_buffer);
    auto util = master->getUtilInterface();

    char type_buf[64];

    if (num_bind_args != info.in_vars_count) {
        throw Php_Firebird_Exception(zend_ce_argument_count_error,
            fmt("Statement expects %u arguments, %u given", info.in_vars_count, num_bind_args));
    }

    for (unsigned int i = 0; i < info.in_vars_count; ++i) {
        zval *b_var = &b_vars[i];
        ZVAL_DEREF(b_var);

        const unsigned argn = i + 1;
        const auto &var = in_vars[i];
        auto sqlind = reinterpret_cast<short*>(in_buffer + var.null_offset);

        if (Z_TYPE_P(b_var) == IS_NULL) {
            *sqlind = -1;
            continue;
        }

        *sqlind = 0;

        auto sqldata = in_buffer + var.offset;
        const auto sqltype = var.type;
        const auto scale = var.scale;
        const auto type_named = fbu_get_sql_type_name(&in_vars[i], type_buf, sizeof(type_buf));
        StrGuard tmp;

        FBDEBUG("sqltype: %s, sql buflen: %d, phptype: %s",
            type_named, var.length, zend_zval_type_name(b_var));

        auto wrong_type = [&]() {
            return Php_Firebird_Exception(zend_ce_type_error,
                fmt("Argument %u: type mismatch '%s' to SQL type '%s'",
                    argn, zend_zval_type_name(b_var), type_named));
        };

        switch (sqltype) {
            case SQL_NULL:
                // `? IS NULL`: only the indicator matters
                break;

            case SQL_SHORT:
            case SQL_LONG:
            case SQL_INT64: {
                const int64_t lmin = sqltype == SQL_SHORT ? INT16_MIN : sqltype == SQL_LONG ? INT32_MIN : INT64_MIN;
                const int64_t lmax = sqltype == SQL_SHORT ? INT16_MAX : sqltype == SQL_LONG ? INT32_MAX : INT64_MAX;
                int64_t v;
                bool fits;

                if (Z_TYPE_P(b_var) == IS_LONG) {
                    fits = scale_int64(Z_LVAL_P(b_var), scale, &v);
                } else if (Z_TYPE_P(b_var) == IS_DOUBLE || Z_TYPE_P(b_var) == IS_STRING) {
                    if (Z_TYPE_P(b_var) == IS_DOUBLE && !zend_finite(Z_DVAL_P(b_var))) {
                        throw Php_Firebird_Exception(zend_ce_value_error,
                            fmt("Argument %u: cannot bind INF or NAN to %s", argn, type_named));
                    }
                    // Firebird's own parser: rounds to the column scale, accepts exponents
                    FB_I128 v128;
                    util->getInt128(&st)->fromString(&st, scale, ZSTR_VAL(as_string(b_var, tmp)), &v128);
                    v = static_cast<int64_t>(v128.fb_data[0]);
                    fits = static_cast<int64_t>(v128.fb_data[1]) == (v < 0 ? -1 : 0);
                } else {
                    throw wrong_type();
                }

                if (!fits || v < lmin || v > lmax) {
                    const zend_string *str = as_string(b_var, tmp);
                    throw Php_Firebird_Exception(zend_ce_value_error,
                        fmt("Argument %u: value %s is out of range for %s", argn, ZSTR_VAL(str), type_named));
                }

                if (sqltype == SQL_SHORT) {
                    *reinterpret_cast<int16_t*>(sqldata) = static_cast<int16_t>(v);
                } else if (sqltype == SQL_LONG) {
                    *reinterpret_cast<int32_t*>(sqldata) = static_cast<int32_t>(v);
                } else {
                    *reinterpret_cast<int64_t*>(sqldata) = v;
                }
            } break;

            case SQL_FLOAT:
            case SQL_DOUBLE: {
                double d;

                if (Z_TYPE_P(b_var) == IS_DOUBLE) {
                    d = Z_DVAL_P(b_var);
                } else if (Z_TYPE_P(b_var) == IS_LONG) {
                    d = static_cast<double>(Z_LVAL_P(b_var));
                } else if (Z_TYPE_P(b_var) == IS_STRING) {
                    zend_long lval;
                    auto t = is_numeric_string(Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), &lval, &d, false);
                    if (!t) {
                        throw Php_Firebird_Exception(zend_ce_value_error,
                            fmt("Argument %u: '%s' is not numeric", argn, Z_STRVAL_P(b_var)));
                    }
                    if (t == IS_LONG) d = static_cast<double>(lval);
                } else {
                    throw wrong_type();
                }

                if (!zend_finite(d)) {
                    throw Php_Firebird_Exception(zend_ce_value_error,
                        fmt("Argument %u: cannot bind INF or NAN to %s", argn, type_named));
                }

                if (sqltype == SQL_FLOAT) {
                    if (d > FLT_MAX || d < -FLT_MAX) {
                        throw Php_Firebird_Exception(zend_ce_value_error,
                            fmt("Argument %u: value %g is out of range for FLOAT", argn, d));
                    }
                    *reinterpret_cast<float*>(sqldata) = static_cast<float>(d);
                } else {
                    *reinterpret_cast<double*>(sqldata) = d;
                }
            } break;

            case SQL_INT128:
            case SQL_DEC16:
            case SQL_DEC34: {
                if (Z_TYPE_P(b_var) != IS_STRING && Z_TYPE_P(b_var) != IS_LONG && Z_TYPE_P(b_var) != IS_DOUBLE) {
                    throw wrong_type();
                }
                if (Z_TYPE_P(b_var) == IS_DOUBLE && !zend_finite(Z_DVAL_P(b_var))) {
                    throw Php_Firebird_Exception(zend_ce_value_error,
                        fmt("Argument %u: cannot bind INF or NAN to %s", argn, type_named));
                }

                const char *str = ZSTR_VAL(as_string(b_var, tmp));

                if (sqltype == SQL_INT128) {
                    util->getInt128(&st)->fromString(&st, scale, str, reinterpret_cast<FB_I128*>(sqldata));
                } else if (sqltype == SQL_DEC16) {
                    util->getDecFloat16(&st)->fromString(&st, str, reinterpret_cast<FB_DEC16*>(sqldata));
                } else {
                    util->getDecFloat34(&st)->fromString(&st, str, reinterpret_cast<FB_DEC34*>(sqldata));
                }
            } break;

            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
            case SQL_TIME_TZ:
            case SQL_TIMESTAMP:
            case SQL_TIMESTAMP_TZ:
                bind_datetime(b_var, argn, var, sqldata, type_named);
                break;

            case SQL_BOOLEAN:
                if (Z_TYPE_P(b_var) == IS_FALSE) {
                    *reinterpret_cast<FB_BOOLEAN*>(sqldata) = FB_FALSE;
                } else if (Z_TYPE_P(b_var) == IS_TRUE) {
                    *reinterpret_cast<FB_BOOLEAN*>(sqldata) = FB_TRUE;
                } else {
                    throw wrong_type();
                }
                break;

            case SQL_BLOB: {
                ISC_QUAD bl_id;
                if (Z_TYPE_P(b_var) == IS_OBJECT && Z_OBJCE_P(b_var) == FireBird_Blob_Id_ce) {
                    bl_id = get_firebird_blob_id_from_zval(b_var)->bl_id;
                } else if (Z_TYPE_P(b_var) == IS_OBJECT && Z_OBJCE_P(b_var) == FireBird_Blob_ce) {
                    bl_id = get_firebird_blob_from_zval(b_var)->info->id;
                } else if (is_stringable(b_var)) {
                    // The cast operates on a const view only: create_blob_from_string()
                    // does not modify the string
                    bl_id = tra.create_blob_from_string(const_cast<zend_string*>(as_string(b_var, tmp)));
                } else {
                    throw wrong_type();
                }
                *reinterpret_cast<ISC_QUAD*>(sqldata) = bl_id;
            } break;

            case SQL_TEXT:
            case SQL_VARYING: {
                if (!is_stringable(b_var)) {
                    throw wrong_type();
                }

                const zend_string *str = as_string(b_var, tmp);

                if (ZSTR_LEN(str) > var.length) {
                    throw Php_Firebird_Exception(zend_ce_value_error,
                        fmt("Argument %u: string of size %zu does not fit buffer of size %u",
                            argn, ZSTR_LEN(str), var.length));
                }

                if (sqltype == SQL_VARYING) {
                    auto buf = reinterpret_cast<firebird_vary*>(sqldata);
                    buf->vary_length = static_cast<unsigned short>(ZSTR_LEN(str));
                    memcpy(buf->vary_string, ZSTR_VAL(str), ZSTR_LEN(str));
                } else {
                    memcpy(sqldata, ZSTR_VAL(str), ZSTR_LEN(str));
                    memset(&sqldata[ZSTR_LEN(str)], ' ', var.length - ZSTR_LEN(str));
                }
            } break;

            case SQL_ARRAY:
                throw Php_Firebird_Exception(zend_ce_value_error,
                    fmt("Argument %u: ARRAY type is not supported", argn));

            default:
                throw Php_Firebird_Exception(zend_ce_value_error,
                    fmt("Argument %u: binding SQL type %s (%u) is not supported", argn, type_named, sqltype));
        }
    }
} // bind

// Binds an int (UNIX timestamp, date-bearing columns only), a string (any format
// strtotime() understands, including fractions and zones) or a DateTimeInterface.
void Statement::bind_datetime(zval *b_var, unsigned argn, const firebird_var &var,
    unsigned char *sqldata, const char *type_named)
{
    auto util = master->getUtilInterface();
    const auto sqltype = var.type;
    const bool has_date = sqltype == SQL_TYPE_DATE || sqltype == SQL_TIMESTAMP || sqltype == SQL_TIMESTAMP_TZ;

    if (Z_TYPE_P(b_var) == IS_LONG) {
        if (!has_date) {
            throw Php_Firebird_Exception(zend_ce_type_error,
                fmt("Argument %u: type mismatch 'int' to SQL type '%s'", argn, type_named));
        }

        time_t tt = static_cast<time_t>(Z_LVAL_P(b_var));
        struct tm t;
        if (!php_gmtime_r(&tt, &t)) {
            throw Php_Firebird_Exception(zend_ce_value_error,
                fmt("Argument %u: could not convert timestamp " ZEND_LONG_FMT, argn, Z_LVAL_P(b_var)));
        }

        ISC_DATE date = util->encodeDate(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        ISC_TIME time = util->encodeTime(t.tm_hour, t.tm_min, t.tm_sec, 0);

        if (sqltype == SQL_TYPE_DATE) {
            *reinterpret_cast<ISC_DATE*>(sqldata) = date;
        } else if (sqltype == SQL_TIMESTAMP) {
            auto ts = reinterpret_cast<ISC_TIMESTAMP*>(sqldata);
            ts->timestamp_date = date;
            ts->timestamp_time = time;
        } else {
            auto ts = reinterpret_cast<ISC_TIMESTAMP_TZ*>(sqldata);
            ts->utc_timestamp.timestamp_date = date;
            ts->utc_timestamp.timestamp_time = time;
            ts->time_zone = 1439; // GMT: Firebird offset ids are minutes + 1439, so 0 means -23:59
        }
        return;
    }

    zval dateo;
    ZVAL_UNDEF(&dateo);
    const timelib_time *t = nullptr;

    if (Z_TYPE_P(b_var) == IS_OBJECT && instanceof_function(Z_OBJCE_P(b_var), php_date_get_interface_ce())) {
        t = Z_PHPDATE_P(b_var)->time;
        if (!t) {
            throw Php_Firebird_Exception(zend_ce_value_error,
                fmt("Argument %u: DateTime object is not initialized", argn));
        }
    } else if (Z_TYPE_P(b_var) == IS_STRING) {
        php_date_instantiate(php_date_get_date_ce(), &dateo);
        if (!php_date_initialize(Z_PHPDATE_P(&dateo), Z_STRVAL_P(b_var), Z_STRLEN_P(b_var), NULL, NULL, 0)) {
            zval_ptr_dtor(&dateo);
            throw Php_Firebird_Exception(zend_ce_value_error,
                fmt("Argument %u: could not parse '%s' as %s", argn, Z_STRVAL_P(b_var), type_named));
        }
        t = Z_PHPDATE_P(&dateo)->time;
    } else {
        throw Php_Firebird_Exception(zend_ce_type_error,
            fmt("Argument %u: type mismatch '%s' to SQL type '%s'", argn, zend_zval_type_name(b_var), type_named));
    }

    // Copy what is needed before the temporary object goes away
    const timelib_sll y = t->y, m = t->m, d = t->d;
    const unsigned h = (unsigned)t->h, mi = (unsigned)t->i, sec = (unsigned)t->s;
    const unsigned fractions = FB_USEC_TO_FRACTIONS(t->us);
    char zone_buf[16];
    std::string zone = zone_name(t, zone_buf, sizeof(zone_buf));

    if (Z_TYPE(dateo) != IS_UNDEF) zval_ptr_dtor(&dateo);

    if (has_date && (y < 1 || y > 9999)) {
        throw Php_Firebird_Exception(zend_ce_value_error,
            fmt("Argument %u: year %lld is out of range for %s", argn, (long long)y, type_named));
    }

    switch (sqltype) {
        case SQL_TYPE_DATE:
            *reinterpret_cast<ISC_DATE*>(sqldata) = util->encodeDate((unsigned)y, (unsigned)m, (unsigned)d);
            break;
        case SQL_TYPE_TIME:
            *reinterpret_cast<ISC_TIME*>(sqldata) = util->encodeTime(h, mi, sec, fractions);
            break;
        case SQL_TIME_TZ:
            util->encodeTimeTz(&st, reinterpret_cast<ISC_TIME_TZ*>(sqldata), h, mi, sec, fractions, zone.c_str());
            break;
        case SQL_TIMESTAMP: {
            auto ts = reinterpret_cast<ISC_TIMESTAMP*>(sqldata);
            ts->timestamp_date = util->encodeDate((unsigned)y, (unsigned)m, (unsigned)d);
            ts->timestamp_time = util->encodeTime(h, mi, sec, fractions);
        } break;
        case SQL_TIMESTAMP_TZ:
            util->encodeTimeStampTz(&st, reinterpret_cast<ISC_TIMESTAMP_TZ*>(sqldata),
                (unsigned)y, (unsigned)m, (unsigned)d, h, mi, sec, fractions, zone.c_str());
            break;
    }
}

bool Statement::is_select() const
{
    return info.statement_type == isc_info_sql_stmt_select ||
        info.statement_type == isc_info_sql_stmt_select_for_upd;
}

// Closes and releases the cursor, if any. The interface is released even when
// close() fails, and the member is cleared first so a throw never leaves it dangling.
void Statement::free_cursor()
{
    if (!cursor) return;

    auto c = cursor;
    cursor = nullptr;

    try {
        c->close(&st);
    } catch (...) {
        c->release();
        throw;
    }
    c->release();
}

// Opens a fresh cursor over the currently bound input, replacing any previous one.
void Statement::open_cursor()
{
    free_cursor();
    cursor_exhausted = false;
    cursor = tra.open_cursor(get(), input_metadata, in_buffer, output_metadata);
}

void Statement::close_cursor()
{
    get_cursor();
    cursor_exhausted = true;
    free_cursor();
}

int Statement::fetch_next()
{
    FBDEBUG("Statement::fetch_next(this=%p, out_buffer=%p)", this, out_buffer);

    // As in Firebird: rows come from what execute returns (the IResultSet of
    // openCursor(), or the output message of IStatement::execute()). A statement
    // that was only prepared has nothing to fetch.
    if (!is_executed) {
        throw Php_Firebird_Exception(zend_ce_error, "Statement must be executed before fetching");
    }

    if (!is_select()) {
        // Non-cursor statements expose their output message (RETURNING,
        // EXECUTE PROCEDURE) as exactly one row, once per execute.
        if (cursor_exhausted) return IStatus::RESULT_NO_DATA;
        cursor_exhausted = true;
        return IStatus::RESULT_OK;
    }

    // Exhausted, or closed with close_cursor()
    if (cursor_exhausted || !cursor) return IStatus::RESULT_NO_DATA;

    auto fetch_res = cursor->fetchNext(&st, out_buffer);
    if (fetch_res != IStatus::RESULT_OK) {
        cursor_exhausted = true;
        free_cursor();
    }
    return fetch_res;
}

void Statement::output_buffer_to_array(int flags, HashTable **hash)
{
    zval *result;

    if (flags & FBP_FETCH_HASHED) {
        if (!ht_aliases) alloc_ht_aliases();
        *hash = zend_array_dup(ht_aliases);
    } else if (flags & FBP_FETCH_INDEXED) {
        if (!ht_ind) alloc_ht_ind();
        *hash = zend_array_dup(ht_ind);
    } else {
        throw Php_Firebird_Exception(zend_ce_error, "BUG: fetch method not set");
    }

    try {
        for (unsigned int index = 0; index < info.out_vars_count; index++) {
            result = zend_hash_get_current_data(*hash);
            var_zval(result, index, flags);
            zend_hash_move_forward(*hash);
        }
    } catch (...) {
        zend_array_destroy(*hash);
        *hash = nullptr;
        throw;
    }

    zend_hash_internal_pointer_reset(*hash);
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

    auto nullind = *(int16_t*)(out_buffer + out_vars[index].null_offset);

    if (nullind) {
        ZVAL_NULL(val);
        return SUCCESS;
    }

    auto type = out_vars[index].type;
    auto len = out_vars[index].length;
    auto scale = out_vars[index].scale;
    auto data = out_buffer + out_vars[index].offset;

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
            fbp_fatal("unhandled type: %d, field: %s", type, out_vars[index].alias);

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
            t_date_obj->time->sse_uptodate = 0;

            // efree(date_obj->time->tz_abbr);
            // efree(date_obj->time);
            if ((flags & FBP_FETCH_UNIXTIME) && !(type == SQL_TYPE_TIME || type == SQL_TIME_TZ)) {
                struct tm tm_time = {0};
                tm_time.tm_year = t_date_obj->time->y - 1900;  // years since 1900
                tm_time.tm_mon  = t_date_obj->time->m - 1;     // 0-11
                tm_time.tm_mday = t_date_obj->time->d;
                tm_time.tm_hour = t_date_obj->time->h;
                tm_time.tm_min  = t_date_obj->time->i;
                tm_time.tm_sec  = t_date_obj->time->s;

                ZVAL_LONG(val, timegm(&tm_time));
            } else if (flags & FBP_FETCH_DATE_OBJ) {
                ZVAL_COPY(val, &t_dateo);
            } else {
                ZVAL_STR(val, php_format_date_obj(t_format, strlen(t_format), t_date_obj));
            }

            zval_ptr_dtor(&t_dateo);
            break;

        case SQL_BLOB:
            if (flags & FBP_FETCH_BLOB_TEXT) {
                ZVAL_STR(val, tra.get_blob_contents((ISC_QUAD *)data));
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
    switch (info.statement_type) {
        // Cursor statements. Firebird 5 also reports every multi-row DML RETURNING
        // (UPDATE, DELETE, MERGE, UPDATE OR INSERT, INSERT ... SELECT) and EXECUTE
        // BLOCK with outputs as select. The DML is applied when the cursor opens.
        case isc_info_sql_stmt_select:
        case isc_info_sql_stmt_select_for_upd:
            open_cursor();
            break;

        // Single-message statements: any output (single-row INSERT ... RETURNING,
        // EXECUTE PROCEDURE) lands in out_buffer and is fetched once
        case isc_info_sql_stmt_insert:
        case isc_info_sql_stmt_update:
        case isc_info_sql_stmt_delete:
        case isc_info_sql_stmt_exec_procedure:
        case isc_info_sql_stmt_set_generator:
        case isc_info_sql_stmt_ddl:             // also SET / ALTER SESSION management statements
        case isc_info_sql_stmt_savepoint:
        case isc_info_sql_stmt_start_trans:     // SET TRANSACTION: adopted by Transaction
        case isc_info_sql_stmt_commit:
        case isc_info_sql_stmt_rollback:
            free_cursor();
            tra.execute_statement(get(), input_metadata, in_buffer, output_metadata, out_buffer);
            cursor_exhausted = false;
            query_statistics();
            break;

        // isc_info_sql_stmt_get_segment / put_segment: embedded-SQL blob cursors,
        // not produced by DSQL
        default:
            throw Php_Firebird_Exception(zend_ce_error,
                "Unsupported statement type " + std::to_string(info.statement_type));
    }

    is_executed = true;
}

Statement::~Statement() noexcept
{
    FBDEBUG("~Statement(this=%p)", this);

    int err = 0;
    try
    {
        free_cursor();
        if (stmt) stmt->free(&st);
    }
    catch (...)
    {
        err = 1;
    }

    if (cursor) {
        cursor->release();
        cursor = nullptr;
    }

    if (stmt) {
        stmt->release();
        stmt = nullptr;
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

    if (err) fbu_handle_exception(__FILE__, __LINE__);
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
    unsigned char req[] = { isc_info_sql_records };
    unsigned char resp[64] = { 0 };

    info.insert_count = 0;
    info.update_count = 0;
    info.delete_count = 0;
    info.select_count = 0;
    info.affected_count = 0;

    get()->getInfo(&st, sizeof(req), req, sizeof(resp), resp);

    const ISC_UCHAR* p = resp;

    if (resp[0] == isc_info_sql_records) {
        p += 3; // Skip current + length. Otherwise expect isc_info_end
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

const firebird_stmt_info *Statement::get_info()
{
    return &info;
}

IStatement *Statement::get()
{
    FBDEBUG("Statement::get()=%p", stmt);
    if (stmt) {
        return stmt;
    }
    throw Php_Firebird_Exception(zend_ce_error, "Invalid statement pointer");
}

IResultSet *Statement::get_cursor()
{
    FBDEBUG("Statement::get_cursor()=%p", cursor);
    if (cursor) {
        return cursor;
    }
    throw Php_Firebird_Exception(zend_ce_error, "Invalid cursor pointer");
}

void Statement::get_var_info(int in, unsigned int index, zval *var_info)
{
    std::vector<firebird_var> *vars;

    if (in) {
        vars = &in_vars;
    } else {
        vars = &out_vars;
    }

    if (index >= (*vars).size()) {
        throw Php_Firebird_Exception(zend_ce_value_error, "Invalid var index");
    }

    char type_buf[64];
    auto var = &(*vars)[index];

    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "field", sizeof("field") - 1, var->field);

    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "alias", sizeof("alias") - 1, var->alias);

    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "relation", sizeof("relation") - 1, var->relation);

    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "type_str", sizeof("type_str") - 1,
        fbu_get_sql_type_name(var, type_buf, sizeof(type_buf)));

    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "type", sizeof("type") - 1, var->type);

    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "subtype", sizeof("subtype") - 1, var->subtype);

    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "length", sizeof("length") - 1, var->length);

    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "scale", sizeof("scale") - 1, var->scale);

    zend_update_property_bool(FireBird_Var_Info_ce, Z_OBJ_P(var_info),
        "is_nullable", sizeof("is_nullable") - 1, var->is_nullable);
}

} // namespace
