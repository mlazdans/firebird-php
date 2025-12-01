/*
  +----------------------------------------------------------------------+
  | Authors: Martins Lazdans <marrtins@dqdp.net>                         |
  +----------------------------------------------------------------------+
*/

#include <firebird/Interface.h>
#include <execinfo.h>
#include <iostream>
#include <cstdlib>
#include <string>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/statement.hpp"
#include "fbp/blob.hpp"
#include "firebird_php.hpp"

extern "C" {
#include "ibase.h"
#include "php.h"
#include "zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/date/php_date.h"
#include "firebird_utils.h"
}

using namespace Firebird;

static void fbu_copy_status(const ISC_STATUS* from, ISC_STATUS* to, size_t maxLength)
{
    for(size_t i=0; i < maxLength; ++i) {
        memcpy(to + i, from + i, sizeof(ISC_STATUS));
        if (from[i] == isc_arg_end) {
            break;
        }
    }
}

void fbu_handle_exception(const char *file, size_t line)
{
    auto eptr = std::current_exception();
    if (!eptr) return;

    std::string error_msg;
    char msg[1024] = {0};
    zval ex;

    object_init_ex(&ex, FireBird_Fb_Exception_ce);

    zend_update_property_string(FireBird_Fb_Exception_ce,
        Z_OBJ(ex), "file_ext", sizeof("file_ext") - 1, file);

    zend_update_property_long(FireBird_Fb_Exception_ce,
        Z_OBJ(ex), "line_ext", sizeof("line_ext") - 1, line);

    try {
        std::rethrow_exception(eptr);
    } catch (const FbException& e) {
        zval rv, errors, ferror_item;
        HashTable *ht_errors;

        array_init(&errors);
        ht_errors = Z_ARRVAL(errors);

        FB_SQLSTATE_STRING sqlstate;
        ISC_LONG msg_len = 0;

        auto status = e.getStatus();
        unsigned state = status->getState();
        unsigned states[] = {IStatus::STATE_ERRORS, IStatus::STATE_WARNINGS};
        const ISC_STATUS* vectors[] = {status->getErrors(), status->getWarnings()};
        ISC_LONG error_code = 0;
        ISC_INT64 error_code_long = 0;

        fb_sqlstate(sqlstate, status->getErrors());
        zend_update_property_stringl(FireBird_Fb_Exception_ce,
            Z_OBJ(ex), "sqlstate", sizeof("sqlstate") - 1, sqlstate, sizeof(sqlstate));

        for (int i = 0; i < 2; ++i)
        {
            if (state & states[i])
            {
                const ISC_STATUS* vector = vectors[i];
                error_code = isc_sqlcode(&vector[0]);
                if (error_code != -999) {
                    error_code_long = isc_portable_integer((const ISC_UCHAR*)(&vector[1]), 4);
                } else {
                    error_code_long = 0;
                }

                zend_update_property_long(FireBird_Fb_Exception_ce,
                    Z_OBJ(ex), "code", sizeof("code") - 1, error_code_long);

                while((msg_len = fb_interpret(msg, sizeof(msg), &vector)) != 0) {
                    object_init_ex(&ferror_item, FireBird_Fb_Error_ce);
                    update_ferr_props(FireBird_Fb_Error_ce, Z_OBJ(ferror_item), msg, msg_len, error_code, error_code_long);
                    zend_hash_next_index_insert(ht_errors, &ferror_item);

                    if (!error_msg.empty())error_msg += "\n";
                    error_msg += msg;

                    error_code = isc_sqlcode(&vector[0]);
                    if (error_code != -999) {
                        error_code_long = isc_portable_integer((const ISC_UCHAR*)(&vector[1]), 4);
                    } else {
                        error_code_long = 0;
                    }
                }
            }
        }

        zend_update_property(FireBird_Fb_Exception_ce,
            Z_OBJ(ex), "errors", sizeof("errors") - 1, &errors);

        zval_ptr_dtor(&errors);
#if 0
        void* buffer[50];
        int nptrs = backtrace(buffer, 50);
        char** symbols = backtrace_symbols(buffer, nptrs);

        std::cerr << "Stack trace:\n";
        for (int i = 0; i < nptrs; i++) {
            std::cerr << symbols[i] << "\n";
        }
        free(symbols);
#endif
    } catch (const Php_Firebird_Exception& error) {
        // TODO: separate database exception from extension exception
        error_msg = error.what();
    } catch (...) {
        error_msg = "Unhandled exception";
    }

    zend_update_property_stringl(FireBird_Fb_Exception_ce,
        Z_OBJ(ex), "message", sizeof("message") - 1, error_msg.c_str(), error_msg.length());

    zend_throw_exception_object(&ex);
}

#define fbu_ex_wrap_start()                       \
    try                                           \
    {
#define fbu_ex_wrap_end() \
        return SUCCESS;                           \
    }                                             \
    catch (...)                                   \
    {                                             \
        fbu_handle_exception(__FILE__, __LINE__); \
        return FAILURE;                           \
    }

const char* fbu_get_sql_type_name(unsigned type)
{
    switch (type)
    {
    case SQL_TEXT:            return "CHAR";
    case SQL_VARYING:         return "VARCHAR";
    case SQL_SHORT:           return "SHORT";
    case SQL_LONG:            return "LONG";
    case SQL_INT64:           return "BIGINT";
    case SQL_FLOAT:           return "FLOAT";
    case SQL_DOUBLE:          return "DOUBLE";
    case SQL_DEC16:           return "DECFLOAT(16)";
    case SQL_DEC34:           return "DECFLOAT(34)";
    case SQL_INT128:          return "INT128";
    case SQL_D_FLOAT:         return "D_FLOAT";
    case SQL_TIMESTAMP:       return "TIMESTAMP";
    case SQL_TIMESTAMP_TZ:    return "TIMESTAMP WITH TIME ZONE";
    case SQL_TIMESTAMP_TZ_EX: return "EXTENDED TIMESTAMP WITH TIME ZONE";
    case SQL_TYPE_DATE:       return "SQL DATE";
    case SQL_TYPE_TIME:       return "TIME";
    case SQL_TIME_TZ:         return "TIME WITH TIME ZONE";
    case SQL_TIME_TZ_EX:      return "EXTENDED TIME WITH TIME ZONE";
    case SQL_BLOB:            return "BLOB";
    case SQL_ARRAY:           return "ARRAY";
    case SQL_QUAD:            return "QUAD";
    case SQL_BOOLEAN:         return "BOOLEAN";
    case SQL_NULL:            return "NULL";
    default:                  return "UNKNOWN";
    }
}

void fbu_xpb_insert_object(IXpbBuilder* xpb, zval *obj, zend_class_entry *ce,
    const firebird_xpb_zmap *xpb_zmap)
{
    zend_string *prop_name = NULL;
    zend_property_info *prop_info = NULL;
    zval rv, *val, *checkval;
    int i;
    ThrowStatusWrapper st(fb_get_master_interface()->getStatus());

    for (int i = 0; i < xpb_zmap->count; i++) {
        prop_name = zend_string_init(xpb_zmap->names[i], strlen(xpb_zmap->names[i]), 1);

#ifdef PHP_DEBUG
        if (!zend_hash_exists(&ce->properties_info, prop_name)) {
            fbp_fatal("BUG! Property %s does not exist for %s::%s. Verify xpb_zmap",
                xpb_zmap->names[i], ZSTR_VAL(ce->name), xpb_zmap->names[i]);
            zend_string_release(prop_name);
            continue;
        }
#endif

        prop_info = zend_get_property_info(ce, prop_name, 0);
        checkval = OBJ_PROP(Z_OBJ_P(obj), prop_info->offset);
        if (Z_ISUNDEF_P(checkval)) {
            FBDEBUG("property: %s is uninitialized", xpb_zmap->names[i]);
            zend_string_release(prop_name);
            continue;
        }

        val = zend_read_property_ex(ce, Z_OBJ_P(obj), prop_name, 0, &rv);
        zend_string_release(prop_name);

        switch (Z_TYPE_P(val)) {
            case IS_STRING:
                FBDEBUG("property: %s is string: `%s`", xpb_zmap->names[i], Z_STRVAL_P(val));
                // (StatusType* status, unsigned char tag, const char* str)
                xpb->insertString(&st, xpb_zmap->tags[i], Z_STRVAL_P(val));
                break;
            case IS_LONG:
                FBDEBUG("property: %s is long: `%u`", xpb_zmap->names[i], Z_LVAL_P(val));
                xpb->insertInt(&st, xpb_zmap->tags[i], (int)Z_LVAL_P(val));
                break;
            case IS_TRUE:
                FBDEBUG("property: %s is true", xpb_zmap->names[i]);
                xpb->insertInt(&st, xpb_zmap->tags[i], 1);
                break;
            case IS_FALSE:
                FBDEBUG("property: %s is false", xpb_zmap->names[i]);
                xpb->insertInt(&st, xpb_zmap->tags[i], 0);
                break;
            case IS_NULL:
                FBDEBUG("property: %s is null", xpb_zmap->names[i]);
                break;
            default:
                fbp_fatal("BUG! Unhandled: type %s for property %s::%s",
                    zend_get_type_by_const(Z_TYPE_P(val)), ZSTR_VAL(ce->name), xpb_zmap->names[i]);
                break;
        }
    }
}

extern "C" {

// Parse NUMERIC() to signed int representation
int fbu_string_to_numeric(const char *s, size_t slen, int scale, uint64_t max,
    int *sign, int *exp, uint64_t *res)
{
    const char* p = s;
    const char *end = s + slen;

    *sign = *exp = *res = 0;

    if (!slen) return STRNUM_PARSE_OK;

    if (*p == '-') {
        *sign = -1;
        p++;
    } else if (*p == '+') {
        *sign = 1;
        p++;
    } else {
        *sign = 1;
    }

    if (*sign == -1) max += 1;

    int fraction = 0;
    uint64_t &r = *res;
    while (p < end) {
        if (*p >= '0' && *p <= '9') {
            r = r * 10 + (*p - '0');
            p++;
            if (fraction) {
                scale++;
                --*exp;
            }
        } else if (*p == '.') {
            if (fraction) return STRNUM_PARSE_ERROR;
            fraction = 1;
            p++;
            continue;
        } else {
            return STRNUM_PARSE_ERROR;
        }

        if (r > max) return STRNUM_PARSE_OVERFLOW;
    }

    while (scale < 0) {
        r *= 10;
        if (r > max) return STRNUM_PARSE_OVERFLOW;
        scale++;
        --*exp;
    }

    return STRNUM_PARSE_OK;
}

// Returns the client version. 0 bytes are minor version, 1 bytes are major version.
unsigned fbu_get_client_version(void)
{
    IUtil* util = fb_get_master_interface()->getUtilInterface();
    return util->getClientVersion();
}

void fbu_init_date_object(const char *tzbuff, zval *o)
{
    zval tzo, tzo_arg;

    php_date_instantiate(php_date_get_date_ce(), o);

    if (tzbuff) {
        ZVAL_STRING(&tzo_arg, tzbuff);
        object_init_with_constructor(&tzo, php_date_get_timezone_ce(), 1, &tzo_arg, nullptr);
        php_date_initialize(Z_PHPDATE_P(o), "", 0, NULL, &tzo, 0);
        zval_ptr_dtor(&tzo_arg);
        zval_ptr_dtor(&tzo);
    } else {
        php_date_initialize(Z_PHPDATE_P(o), "", 0, NULL, NULL, 0);
    }

    // zend_string *tt = php_format_date(ff, strlen(ff), ((ISC_TIME_TZ *)data)->utc_time, 0);
    // php_date_time_set(&new_object, h, i, s, ms, return_value);
    // php_date_initialize_from_ts_long(date_obj, 0, 0);
}



using namespace FBP;

int fbu_is_valid_dbh(size_t dbh)
{
    return (dbh && dbh <= FBG(db_list).size() && FBG(db_list)[dbh - 1]);
}

static inline std::unique_ptr<Database>& get_db_ptr(std::size_t h, const char *file_name, size_t line_num)
{
    if (fbu_is_valid_dbh(h)) {
        return FBG(db_list)[h - 1];
    } else {
        throw Php_Firebird_Exception(zend_ce_error, "Invalid database handle");
    }
}

#define get_db(h) get_db_ptr(h, __FILE__, __LINE__)

int fbu_is_valid_trh(size_t dbh, size_t trh)
{
    try {
        get_db(dbh)->get_transaction(trh);
        return 1;
    } catch(...) {
        return 0;
    }
}

int fbu_is_valid_sth(size_t dbh, size_t sth)
{
    try {
        get_db(dbh)->get_statement(sth);
        return 1;
    } catch(...) {
        return 0;
    }
}

int fbu_database_init(size_t *dbh)
{
    FBDEBUG("%s()", __func__);
    fbu_ex_wrap_start();
        FBG(db_list).emplace_back(new Database());
        *dbh = FBG(db_list).size();
        FBDEBUG("    dbh=%zu", *dbh);
    fbu_ex_wrap_end();
}

int fbu_database_connect(size_t dbh, zval *args)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh)->connect(args);
    fbu_ex_wrap_end();
}

int fbu_database_create(size_t dbh, zval *args)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh)->create(args);
    fbu_ex_wrap_end();
}

int fbu_database_disconnect(size_t dbh)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh)->disconnect();
        get_db(dbh).reset();
    fbu_ex_wrap_end();
}

int fbu_database_drop(size_t dbh)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh)->drop();
        get_db(dbh).reset();
    fbu_ex_wrap_end();
}

int fbu_database_execute_create(size_t dbh, unsigned int len_sql, const char *sql)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh)->execute_create(len_sql, sql);
    fbu_ex_wrap_end();
}

int fbu_database_free(size_t dbh)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh).reset();
    fbu_ex_wrap_end();
}

int fbu_transaction_init(size_t dbh, size_t *trh)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        *trh = get_db(dbh)->transaction_init();
    fbu_ex_wrap_end();
}

int fbu_transaction_get_info(size_t dbh, size_t trh, const firebird_trans_info **info)
{
    FBDEBUG("%s(dbh=%zu, trh=%zu)", __func__, dbh, trh);
    fbu_ex_wrap_start();
        *info = get_db(dbh)->get_transaction(trh)->get_info();
    fbu_ex_wrap_end();
}

int fbu_transaction_free(size_t dbh, size_t trh)
{
    FBDEBUG("%s(dbh=%zu, trh=%zu)", __func__, dbh, trh);
    fbu_ex_wrap_start();
        get_db(dbh)->transaction_free(trh);
    fbu_ex_wrap_end();
}

int fbu_transaction_finalize(size_t dbh, size_t trh, int mode)
{
    FBDEBUG("%s(dbh=%zu, trh=%zu)", __func__, dbh, trh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_transaction(trh)->finalize(mode);
    fbu_ex_wrap_end();
}

int fbu_transaction_start(size_t dbh, size_t trh, const firebird_tbuilder *builder)
{
    FBDEBUG("%s(dbh=%zu, trh=%zu)", __func__, dbh, trh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_transaction(trh)->start(builder);
    fbu_ex_wrap_end();
}

int fbu_transaction_execute(size_t dbh, size_t trh, size_t len_sql, const char *sql)
{
    FBDEBUG("%s(dbh=%zu, trh=%zu)", __func__, dbh, trh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_transaction(trh)->execute(len_sql, sql);
    fbu_ex_wrap_end();
}


int fbu_statement_init(size_t dbh, size_t trh, size_t *sth)
{
    FBDEBUG("%s(dbh=%zu, trh=%zu)", __func__, dbh, trh);
    fbu_ex_wrap_start();
        *sth = get_db(dbh)->statement_init(trh);
    fbu_ex_wrap_end();
}

int fbu_statement_get_info(size_t dbh, size_t sth, const firebird_stmt_info **info)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        *info = get_db(dbh)->get_statement(sth)->get_info();
    fbu_ex_wrap_end();
}

int fbu_statement_free(size_t dbh, size_t sth)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->statement_free(sth);
    fbu_ex_wrap_end();
}

int fbu_statement_prepare(size_t dbh, size_t sth, unsigned len_sql, const char *sql)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->prepare(len_sql, sql);
    fbu_ex_wrap_end();
}

int fbu_statement_bind(size_t dbh, size_t sth, zval *b_vars, unsigned int num_bind_args)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->bind(b_vars, num_bind_args);
    fbu_ex_wrap_end();
}

int fbu_statement_open_cursor(size_t dbh, size_t sth)
{
    // TODO: check if already open
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->open_cursor();
    fbu_ex_wrap_end();
}

int fbu_statement_fetch_next(size_t dbh, size_t sth, int *istatus)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        *istatus = get_db(dbh)->get_statement(sth)->fetch_next();
    fbu_ex_wrap_end();
}

int fbu_statement_output_buffer_to_array(size_t dbh, size_t sth, int flags, HashTable **ht)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->output_buffer_to_array(flags, ht);
    fbu_ex_wrap_end();
}

int fbu_statement_execute(size_t dbh, size_t sth)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->execute();
    fbu_ex_wrap_end();
}

int fbu_statement_close_cursor(size_t dbh, size_t sth)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->close_cursor();
    fbu_ex_wrap_end();
}

int fbu_blob_init(size_t dbh, size_t trh, size_t *blh)
{
    FBDEBUG("%s(dbh=%zu, trh=%zu)", __func__, dbh, trh);
    fbu_ex_wrap_start();
        *blh = get_db(dbh)->blob_init(trh);
    fbu_ex_wrap_end();
}

int fbu_blob_get_info(size_t dbh, size_t blh, const firebird_blob_info **info)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        *info = get_db(dbh)->get_blob(blh)->get_info();
    fbu_ex_wrap_end();
}

int fbu_blob_create(size_t dbh, size_t blh)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_blob(blh)->create();
    fbu_ex_wrap_end();
}

int fbu_blob_open(size_t dbh, size_t blh, ISC_QUAD *bl_id)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_blob(blh)->open(bl_id);
    fbu_ex_wrap_end();
}

int fbu_blob_free(size_t dbh, size_t blh)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        get_db(dbh)->blob_free(blh);
    fbu_ex_wrap_end();
}

int fbu_blob_put(size_t dbh, size_t blh, unsigned int buf_size, const char *buf)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_blob(blh)->put_contents(buf_size, buf);
    fbu_ex_wrap_end();
}

int fbu_blob_close(size_t dbh, size_t blh)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_blob(blh)->close();
    fbu_ex_wrap_end();
}

int fbu_blob_cancel(size_t dbh, size_t blh)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_blob(blh)->cancel();
    fbu_ex_wrap_end();
}

int fbu_blob_get(size_t dbh, size_t blh, int max_len, zend_string **buf)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_blob(blh)->get_contents(max_len, buf);
    fbu_ex_wrap_end();
}

int fbu_blob_seek(size_t dbh, size_t blh, int mode, int offset, int *new_offset)
{
    FBDEBUG("%s(dbh=%zu, blh=%zu)", __func__, dbh, blh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_blob(blh)->seek(mode, offset, new_offset);
    fbu_ex_wrap_end();
}

} // extern "C"

