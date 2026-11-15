/*
  +----------------------------------------------------------------------+
  | Authors: Martins Lazdans <marrtins@dqdp.net>                         |
  +----------------------------------------------------------------------+
*/

#include <firebird/Interface.h>
// #include <execinfo.h>
#include <iostream>
#include <cstdlib>
#include <string>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/service.hpp"
#include "fbp/statement.hpp"
#include "fbp/blob.hpp"
#include "firebird_php.hpp"
#include "firebird_utils.h"

extern "C" {
#include "ibase.h"
#include "php.h"
#include "zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/date/php_date.h"
}

using namespace Firebird;

// Destructors run from free_obj or during request shutdown, where throwing a PHP
// exception is unsafe (no stack frame at shutdown). Report the error as a warning.
void fbu_report_exception_as_warning(const char *file, size_t line)
{
    auto eptr = std::current_exception();
    if (!eptr) return;

    std::string error_msg;
    try {
        std::rethrow_exception(eptr);
    } catch (const FbException& e) {
        char msg[1024];
        fb_get_master_interface()->getUtilInterface()->formatStatus(msg, sizeof(msg), e.getStatus());
        error_msg = msg;
    } catch (const std::exception& e) {
        error_msg = e.what();
    } catch (...) {
        error_msg = "Unhandled exception";
    }

#ifdef PHP_DEBUG
    php_error_docref(NULL, E_WARNING, "%s (%s:%zu)", error_msg.c_str(), file, line);
#else
    (void)file; (void)line;
    php_error_docref(NULL, E_WARNING, "%s", error_msg.c_str());
#endif
}

static std::string fbu_fb_message(ISC_STATUS code)
{
    const ISC_STATUS err[] = { isc_arg_gds, code, isc_arg_end };
    const ISC_STATUS *vector = err;
    char msg[512];

    if (fb_interpret(msg, sizeof(msg), &vector)) {
        return msg;
    }

    return "Firebird error " + std::to_string(code);
}

Php_Firebird_Exception::Php_Firebird_Exception(zend_class_entry *ce, ISC_STATUS fb_code)
    : std::runtime_error(fbu_fb_message(fb_code)),
    ce(ce),
    fb_code(fb_code)
{
}

// Instance of ce (Fb_Exception or a subclass) with message, code and the extension
// source location set
static void fbu_init_exception(zval *ex, zend_class_entry *ce, const char *file, size_t line,
    const char *msg, size_t msg_len, zend_long code)
{
    if (!ce || !instanceof_function(ce, FireBird_Fb_Exception_ce)) {
        ce = FireBird_Fb_Exception_ce;
    }

    object_init_ex(ex, ce);
    zend_update_property_stringl(FireBird_Fb_Exception_ce, Z_OBJ_P(ex), "message", sizeof("message") - 1, msg, msg_len);
    zend_update_property_long(FireBird_Fb_Exception_ce, Z_OBJ_P(ex), "code", sizeof("code") - 1, code);
    zend_update_property_string(FireBird_Fb_Exception_ce, Z_OBJ_P(ex), "file_ext", sizeof("file_ext") - 1, file);
    zend_update_property_long(FireBird_Fb_Exception_ce, Z_OBJ_P(ex), "line_ext", sizeof("line_ext") - 1, line);
}

void fbu_throw(zend_class_entry *ce, const char *file, size_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    zend_string *msg = zend_vstrpprintf(0, format, args);
    va_end(args);

    zval ex;
    fbu_init_exception(&ex, ce, file, line, ZSTR_VAL(msg), ZSTR_LEN(msg), 0);
    zend_string_release(msg);

    zend_throw_exception_object(&ex);
}

// One Fb_Status_Item per message in a status vector. Returns the messages joined
// with "\n" and the first item's code.
static std::string fbu_status_items(const ISC_STATUS *vector, zval *items, zend_long *first_code)
{
    std::string joined;
    char msg[1024];

    array_init(items);

    for (;;) {
        // The code fb_interpret() is about to format; it skips a leading SQLSTATE too
        if (vector[0] == isc_arg_sql_state) vector += 2;

        zend_long code = 0;
        if (vector[0] == isc_arg_gds && vector[1] == 0 && vector[2] == isc_arg_warning) {
            code = vector[3];
        } else if (vector[0] == isc_arg_gds || vector[0] == isc_arg_warning) {
            code = vector[1];
        }

        ISC_LONG len = fb_interpret(msg, sizeof(msg), &vector);
        if (!len) break;

        if (first_code && zend_hash_num_elements(Z_ARRVAL_P(items)) == 0) {
            *first_code = code;
        }

        zval item;
        object_init_ex(&item, FireBird_Fb_Status_Item_ce);
        zend_update_property_long(FireBird_Fb_Status_Item_ce, Z_OBJ(item), "code", sizeof("code") - 1, code);
        zend_update_property_stringl(FireBird_Fb_Status_Item_ce, Z_OBJ(item), "message", sizeof("message") - 1, msg, len);
        zend_hash_next_index_insert(Z_ARRVAL_P(items), &item);

        if (!joined.empty()) joined += "\n";
        joined.append(msg, len);
    }

    return joined;
}

void fbu_handle_exception(const char *file, size_t line)
{
    auto eptr = std::current_exception();
    if (!eptr) return;

    zend_class_entry *ce = FireBird_Fb_Exception_ce;
    std::string error_msg;
    zend_long code = 0;
    zval ex;

    try {
        std::rethrow_exception(eptr);
    } catch (const FbException& e) {
        auto status = e.getStatus();
        unsigned state = status->getState();
        zval items, warnings;
        FB_SQLSTATE_STRING sqlstate = {0};

        fb_sqlstate(sqlstate, status->getErrors());

        if (state & IStatus::STATE_ERRORS) {
            error_msg = fbu_status_items(status->getErrors(), &items, &code);
                } else {
            array_init(&items);
        }

        if (state & IStatus::STATE_WARNINGS) {
            fbu_status_items(status->getWarnings(), &warnings, nullptr);
                    } else {
            array_init(&warnings);
        }

        fbu_init_exception(&ex, FireBird_Fb_Server_Exception_ce, file, line, error_msg.c_str(), error_msg.length(), code);
        zend_update_property_stringl(FireBird_Fb_Server_Exception_ce, Z_OBJ(ex), "sqlstate", sizeof("sqlstate") - 1,
            sqlstate, strnlen(sqlstate, sizeof(sqlstate)));
        zend_update_property(FireBird_Fb_Server_Exception_ce, Z_OBJ(ex), "status", sizeof("status") - 1, &items);
        zend_update_property(FireBird_Fb_Server_Exception_ce, Z_OBJ(ex), "warnings", sizeof("warnings") - 1, &warnings);
        zval_ptr_dtor(&items);
        zval_ptr_dtor(&warnings);

        zend_throw_exception_object(&ex);
        return;
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
        ce = error.ce;
        code = error.fb_code;
        error_msg = error.what();
    } catch (const std::bad_alloc&) {
        error_msg = "Out of memory";
    } catch (const std::exception& e) {
        error_msg = e.what();
    } catch (...) {
        error_msg = "Unhandled exception";
    }

    fbu_init_exception(&ex, ce, file, line, error_msg.c_str(), error_msg.length(), code);
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

// +-----------+------------------------+---------------------+
// | Precision | Data type              | Dialect 3           |
// +-----------+------------------------+---------------------+
// | 1 - 4     | NUMERIC                | SMALLINT            |
// | 1 - 4     | DECIMAL                | INTEGER             |
// | 5 - 9     | NUMERIC or DECIMAL     | INTEGER             |
// | 10 - 18   | NUMERIC or DECIMAL     | BIGINT              |
// | 19 - 38   | NUMERIC or DECIMAL     | INT128              |
// +-----------+------------------------+---------------------+
const char* fbu_get_sql_type_name(firebird_var *var, char *buf, size_t buf_size)
{
    unsigned short precision = 0;

    if (var->scale < 0) {
        switch (var->type) {
            case SQL_SHORT  : precision = 4; break;
            case SQL_LONG   : precision = 9; break;
            case SQL_INT64  : precision = 18; break;
            case SQL_INT128 : precision = 38; break;
            case SQL_DOUBLE : precision = 15; break; // dialect 1 NUMERIC
        }

        if (precision) {
            slprintf(buf, buf_size, "NUMERIC(%d,%d)", precision, -var->scale);
            return buf;
        }
    }

    switch (var->type)
    {
        case SQL_TEXT:            return "CHAR";
        case SQL_VARYING:         return "VARCHAR";
        case SQL_SHORT:           return "SMALLINT";
        case SQL_LONG:            return "INTEGER";
        case SQL_INT64:           return "BIGINT";
        case SQL_FLOAT:           return "FLOAT";
        case SQL_DOUBLE:          return "DOUBLE PRECISION";
        case SQL_DEC16:           return "DECFLOAT(16)";
        case SQL_DEC34:           return "DECFLOAT(34)";
        case SQL_INT128:          return "INT128";
        case SQL_D_FLOAT:         return "D_FLOAT";
        case SQL_TIMESTAMP:       return "TIMESTAMP";
        case SQL_TIMESTAMP_TZ:    return "TIMESTAMP WITH TIME ZONE";
        case SQL_TIMESTAMP_TZ_EX: return "EXTENDED TIMESTAMP WITH TIME ZONE";
        case SQL_TYPE_DATE:       return "DATE";
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

// Reads a property that must hold a non-empty string, e.g. Connect_Args::$database.
// Throws when it is unset or empty, instead of handing a NULL zval's garbage pointer
// to Firebird.
const char *fbu_read_required_string_prop(zend_class_entry *ce, zval *obj, const char *name)
{
    zval rv;
    zval *val = zend_read_property(ce, Z_OBJ_P(obj), name, strlen(name), 1, &rv);

    if (!val || Z_TYPE_P(val) != IS_STRING || !Z_STRLEN_P(val)) {
        throw Php_Firebird_Exception(FireBird_Fb_Value_Exception_ce,
            std::string(ZSTR_VAL(ce->name)) + "::$" + name + " must be set");
    }

    return Z_STRVAL_P(val);
}

// The Database behind a handle, or nullptr when the handle is not (or no longer) valid
FBP::Database *fbu_get_database(size_t dbh)
{
    return fbu_is_valid_dbh(dbh) ? FBG(db_list).get(dbh).get() : nullptr;
}

void fbu_xpb_insert_object(IXpbBuilder* xpb, zval *obj, zend_class_entry *ce,
    const firebird_xpb_zmap *xpb_zmap)
{
    FBP::ScopedStatus st;

    for (int i = 0; i < xpb_zmap->count; i++) {
        const char *name = xpb_zmap->names[i];
        auto prop_info = static_cast<zend_property_info *>(
            zend_hash_str_find_ptr(&ce->properties_info, name, strlen(name)));

        if (!prop_info || (prop_info->flags & ZEND_ACC_STATIC)) {
            throw Php_Firebird_Exception(FireBird_Fb_Exception_ce,
                std::string("BUG: property ") + ZSTR_VAL(ce->name) + "::$" + name +
                " does not exist. Verify xpb_zmap");
        }

        // Typed properties without a default stay UNDEF until assigned: skip them
        zval *val = OBJ_PROP(Z_OBJ_P(obj), prop_info->offset);
        ZVAL_DEREF(val);
        if (Z_ISUNDEF_P(val)) {
            FBDEBUG("property: %s is uninitialized", name);
            continue;
        }

        switch (Z_TYPE_P(val)) {
            case IS_STRING:
                FBDEBUG("property: %s is string: `%s`", name, Z_STRVAL_P(val));
                xpb->insertString(&st, xpb_zmap->tags[i], Z_STRVAL_P(val));
                break;
            case IS_LONG:
                FBDEBUG("property: %s is long: `" ZEND_LONG_FMT "`", name, Z_LVAL_P(val));
                if (Z_LVAL_P(val) < INT32_MIN || Z_LVAL_P(val) > INT32_MAX) {
                    throw Php_Firebird_Exception(FireBird_Fb_Value_Exception_ce,
                        std::string(ZSTR_VAL(ce->name)) + "::$" + name + " is out of range");
                }
                xpb->insertInt(&st, xpb_zmap->tags[i], (int)Z_LVAL_P(val));
                break;
            case IS_TRUE:
                FBDEBUG("property: %s is true", name);
                xpb->insertInt(&st, xpb_zmap->tags[i], 1);
                break;
            case IS_FALSE:
                FBDEBUG("property: %s is false", name);
                xpb->insertInt(&st, xpb_zmap->tags[i], 0);
                break;
            case IS_NULL:
                FBDEBUG("property: %s is null", name);
                break;
            default:
                throw Php_Firebird_Exception(FireBird_Fb_Value_Exception_ce,
                    std::string("Unsupported type ") + zend_get_type_by_const(Z_TYPE_P(val)) +
                    " for property " + ZSTR_VAL(ce->name) + "::$" + name);
        }
    }
}

extern "C" {

// Returns the client version. 0 bytes are minor version, 1 bytes are major version.
unsigned fbu_get_client_version(void)
{
    IUtil* util = fb_get_master_interface()->getUtilInterface();
    return util->getClientVersion();
}

using namespace FBP;

int fbu_is_valid_dbh(size_t dbh)
{
    return FBG(db_list).is_valid(dbh);
}

static inline std::unique_ptr<Database>& get_db_ptr(std::size_t h, const char *file_name, size_t line_num)
noexcept(false)
{
    if (fbu_is_valid_dbh(h)) {
        return FBG(db_list).get(h);
    } else {
        throw Php_Firebird_Exception(FireBird_Fb_Logic_Exception_ce, isc_bad_db_handle);
    }
}

int fbu_is_valid_svh(size_t svh)
{
    return FBG(sv_list).is_valid(svh);
}

static inline std::unique_ptr<Service>& get_sv_ptr(std::size_t h, const char *file_name, size_t line_num)
noexcept(false)
{
    if (fbu_is_valid_svh(h)) {
        return FBG(sv_list).get(h);
    } else {
        throw Php_Firebird_Exception(FireBird_Fb_Logic_Exception_ce, isc_bad_svc_handle);
    }
}

#define get_db(h) get_db_ptr(h, __FILE__, __LINE__)
#define get_sv(h) get_sv_ptr(h, __FILE__, __LINE__)

int fbu_is_valid_trh(size_t dbh, size_t trh)
{
    try {
        get_db(dbh)->get_transaction(trh);
        return 1;
    } catch(...) {
        return 0;
    }
}

int fbu_is_valid_blh(size_t dbh, size_t blh)
{
    try {
        get_db(dbh)->get_blob(blh);
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
        *dbh = FBG(db_list).add(std::make_unique<FBP::Database>());
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
        FBG(db_list).free(dbh);
    fbu_ex_wrap_end();
}

int fbu_database_drop(size_t dbh)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh)->drop();
        FBG(db_list).free(dbh);
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
        get_db(dbh);
        FBG(db_list).free(dbh);
    fbu_ex_wrap_end();
}

int fbu_database_get_info(size_t dbh, zval *db_info)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_info(db_info);
    fbu_ex_wrap_end();
}

int fbu_transaction_init(size_t dbh, size_t *trh)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        *trh = get_db(dbh)->transaction_init();
    fbu_ex_wrap_end();
}

int fbu_transaction_get_limbo(size_t dbh, zval *tr_arr, short max_count)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_limbo_transactions(tr_arr, max_count);
    fbu_ex_wrap_end();
}

int fbu_transaction_reconnect(size_t dbh, ISC_ULONG id, size_t *trh)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        *trh = get_db(dbh)->transaction_reconnect(id);
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

int fbu_transaction_prepare(size_t dbh, size_t trh)
{
    FBDEBUG("%s(dbh=%zu, trh=%zu)", __func__, dbh, trh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_transaction(trh)->prepare();
    fbu_ex_wrap_end();
}

int fbu_statement_init(size_t dbh, size_t *sth)
{
    FBDEBUG("%s(dbh=%zu)", __func__, dbh);
    fbu_ex_wrap_start();
        *sth = get_db(dbh)->statement_init();
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

int fbu_statement_prepare(size_t dbh, size_t sth, size_t trh, unsigned len_sql, const char *sql)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu, trh=%zu)", __func__, dbh, sth, trh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->prepare(trh, len_sql, sql);
    fbu_ex_wrap_end();
}

int fbu_statement_bind(size_t dbh, size_t sth, size_t trh, zval *b_vars, unsigned int num_bind_args)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu, trh=%zu)", __func__, dbh, sth, trh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->bind(trh, b_vars, num_bind_args);
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

int fbu_statement_execute(size_t dbh, size_t sth, size_t trh)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu, trh=%zu)", __func__, dbh, sth, trh);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->execute(trh);
    fbu_ex_wrap_end();
}

int fbu_statement_close_cursor(size_t dbh, size_t sth)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->close_cursor();
    fbu_ex_wrap_end();
}

int fbu_statement_set_scrollable(size_t dbh, size_t sth)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->set_scrollable();
    fbu_ex_wrap_end();
}

int fbu_statement_fetch_scroll(size_t dbh, size_t sth, int direction, int position, int *istatus)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu, direction=%d, position=%d)", __func__, dbh, sth, direction, position);
    fbu_ex_wrap_start();
        *istatus = get_db(dbh)->get_statement(sth)->fetch_scroll(direction, position);
    fbu_ex_wrap_end();
}

int fbu_statement_is_bof(size_t dbh, size_t sth, bool *result)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        *result = get_db(dbh)->get_statement(sth)->is_bof();
    fbu_ex_wrap_end();
}

int fbu_statement_is_eof(size_t dbh, size_t sth, bool *result)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        *result = get_db(dbh)->get_statement(sth)->is_eof();
    fbu_ex_wrap_end();
}

int fbu_statement_record_count(size_t dbh, size_t sth, ISC_INT64 *count)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        *count = get_db(dbh)->get_statement(sth)->record_count();
    fbu_ex_wrap_end();
}

int fbu_statement_get_var_info(size_t dbh, size_t sth, int in, unsigned int index, zval *var_info)
{
    FBDEBUG("%s(dbh=%zu, sth=%zu)", __func__, dbh, sth);
    fbu_ex_wrap_start();
        get_db(dbh)->get_statement(sth)->get_var_info(in, index, var_info);
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

int fbu_blob_put(size_t dbh, size_t blh, size_t buf_size, const char *buf)
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

int fbu_blob_get(size_t dbh, size_t blh, int64_t max_len, zend_string **buf)
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

int fbu_service_init(size_t *svh)
{
    FBDEBUG("%s()", __func__);
    fbu_ex_wrap_start();
        *svh = FBG(sv_list).add(std::make_unique<FBP::Service>());
        FBDEBUG("    svh=%zu", *svh);
    fbu_ex_wrap_end();
}

int fbu_service_free(size_t svh)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh);
        FBG(sv_list).free(svh);
    fbu_ex_wrap_end();
}

int fbu_service_connect(size_t svh, zval *args)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh)->connect(args);
    fbu_ex_wrap_end();
}

int fbu_service_disconnect(size_t svh)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh)->disconnect();
        FBG(sv_list).free(svh);
    fbu_ex_wrap_end();
}

int fbu_service_get_server_info(size_t svh, zval *Server_Info)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh)->get_info(Server_Info);
    fbu_ex_wrap_end();
}

// User management disabled, see Service::get_users() in fbp/service.cpp
#if 0
int fbu_service_get_users(size_t svh, zval *users)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh)->get_users(users);
    fbu_ex_wrap_end();
}
int fbu_service_add_user(size_t svh, zval *user_info)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh)->add_user(user_info);
    fbu_ex_wrap_end();
}

int fbu_service_modify_user(size_t svh, zval *user_info)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh)->modify_user(user_info);
    fbu_ex_wrap_end();
}

int fbu_service_delete_user(size_t svh, const char *username, size_t username_len)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh)->delete_user(username, username_len);
    fbu_ex_wrap_end();
}

#endif

int fbu_service_get_db_info(size_t svh, zval *server_db_info)
{
    FBDEBUG("%s(svh=%zu)", __func__, svh);
    fbu_ex_wrap_start();
        get_sv(svh)->get_db_info(server_db_info);
    fbu_ex_wrap_end();
}

} // extern "C"

