/*
  +----------------------------------------------------------------------+
  | Authors: Martins Lazdans <marrtins@dqdp.net>                         |
  +----------------------------------------------------------------------+
*/

#include <firebird/Interface.h>
#include <execinfo.h>
#include <iostream>
#include <cstdlib>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/statement.hpp"
#include "fbp/blob.hpp"

extern "C" {
#include "ibase.h"
#include "php.h"
#include "zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/date/php_date.h"
#include "database.h"
#include "firebird_utils.h"
#include "transaction.h"
#include "statement.h"
#include "blob.h"
#include "php_firebird_includes.h"
}

using namespace Firebird;

// struct TransactionList {
//     std::vector<Transaction*> trans;
//     ~TransactionList() {
//         FBDEBUG("[~TransactionList] %d", trans.size());
//         ThrowStatusWrapper st(fb_get_master_interface()->getStatus());

//         for (auto &tx : trans) {
//             FBDEBUG("[*] Closing transaction: %p", tx);
//             if (tx) {
//                 FBDEBUG("Rolling back transaction: %p", tx);
//                 // tx->rollback(&st);
//                 // tx->release();
//                 // tx = nullptr;
//             }
//         }
//     }
// };

static IMaster* fbu_master = fb_get_master_interface();

static void fbu_copy_status(const ISC_STATUS* from, ISC_STATUS* to, size_t maxLength)
{
    for(size_t i=0; i < maxLength; ++i) {
        memcpy(to + i, from + i, sizeof(ISC_STATUS));
        if (from[i] == isc_arg_end) {
            break;
        }
    }
}

void fbu_handle_exception2()
{
    auto eptr = std::current_exception();
    if (!eptr) return;

    try {
        std::rethrow_exception(eptr);
    } catch (const FbException& e) {
        char msg[1024];
        auto util = fb_get_master_interface()->getUtilInterface();
        util->formatStatus(msg, sizeof(msg), e.getStatus());
        fbp_warning("Error: %s\n", msg);
#if 1
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
        fbp_warning("Catched Php_Firebird_Exception: %s\n", error.what());
    } catch (...) {
        fbp_fatal("Unhandled exception");
    }
}

void fbu_handle_exception23()
{
    try
    {
        throw;
    }
    catch (const FbException& e)
    {
        char msg[1024];
        auto util = fb_get_master_interface()->getUtilInterface();
        util->formatStatus(msg, sizeof(msg), e.getStatus());
        fbp_warning("Error: %s\n", msg);
#if 1
        void* buffer[50];
        int nptrs = backtrace(buffer, 50);
        char** symbols = backtrace_symbols(buffer, nptrs);

        std::cerr << "Stack trace:\n";
        for (int i = 0; i < nptrs; i++) {
            std::cerr << symbols[i] << "\n";
        }
        free(symbols);
#endif
    }
    catch (const Php_Firebird_Exception& error)
    {
        fbp_warning("Catched Php_Firebird_Exception: %s\n", error.what());
    }
    catch (...)
    {
        fbp_fatal("Unhandled exception");
    }
}

static void fbu_handle_exception(ThrowStatusWrapper *st, ISC_STATUS* status)
{
    try
    {
        throw;
    }
    catch (const FbException& e)
    {
        fbu_copy_status((const ISC_STATUS*)e.getStatus()->getErrors(), status, 20);
        // fbu_copy_status((const ISC_STATUS*)st->getErrors(), status, 20);
        // char msg[1024];
        // auto fb_util = fbu_master->getUtilInterface();
        // fb_util->formatStatus(msg, sizeof(msg), e.getStatus());
        // php_printf("Error: %s\n", msg);
    }
    catch (const Php_Firebird_Exception& error)
    {
        php_printf("Catched Php_Firebird_Exception: %s\n", error.what());
    }
    catch (...)
    {
        fbp_fatal("Unhandled exception");
    }
}

template<typename Func>
int fbu_call_void(Func&& f)
{
    ThrowStatusWrapper st(fb_get_master_interface()->getStatus());

    try
    {
        return f();
    }
    catch (...)
    {
        fbu_handle_exception2();
        return FAILURE;
    }
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
    ThrowStatusWrapper st(fbu_master->getStatus());

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

/*
class CopyStatusWrapper : public BaseStatusWrapper<ThrowStatusWrapper>
{
public:
    // CopyStatusWrapper(IStatus* aStatus) noexcept
    //     : BaseStatusWrapper(aStatus)
    // {
    // }

    CopyStatusWrapper(IStatus* aStatus, ISC_STATUS* iscStatus)
        : BaseStatusWrapper(aStatus), iscStatusArray(iscStatus)
    {
        std::memset(iscStatusArray, 0, sizeof(ISC_STATUS) * 20);
        iscStatusArray[0] = isc_arg_end;
    }

    static void checkException(CopyStatusWrapper* status)
    {
        if (status->dirty && (status->getState() & IStatus::STATE_ERRORS)) {
            const ISC_STATUS *from = status->getErrors();
            for(size_t i=0; i < 20; ++i) {
                // fbu_copy_status((const ISC_STATUS*)st.getErrors(), status, 20);
                status->iscStatusArray[i] = from[i];
                // memcpy(status->iscStatusArray + i, from + i, sizeof(ISC_STATUS));
                if (from[i] == isc_arg_end) {
                    break;
                }
            }
        }
    }

    // ~CopyStatusWrapper()
    // {
    //     if (status)
    //         status->dispose();
    // }

    // Return pointer to Firebird IStatus* (for API calls)
    // IStatus* operator->() { return fbStatus; }
    // IStatus* get() { return fbStatus; }

    // After a call, copy status back to legacy array
    void sync()
    {
        const ISC_STATUS* fb = status->getErrors();

        if (!fb) return;

        // Copy up to 20 status codes
        for (int i = 0; i < 20 && fb[i] != isc_arg_end; ++i)
            iscStatusArray[i] = fb[i];

        // Terminate the array
        iscStatusArray[19] = isc_arg_end;
    }

private:
    ISC_STATUS* iscStatusArray;
};
*/

extern "C" {

// Parse NUMERIC() to signed int representation
int fbu_string_to_numeric(const char *s, const size_t slen, int scale, uint64_t max,
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
    IUtil* util = fbu_master->getUtilInterface();
    return util->getClientVersion();
}

#if 0
int fbu_create_database(ISC_STATUS* status, firebird_db *db, zval *Create_Args, zend_class_entry *ce)
{
    auto util = fbu_master->getUtilInterface();
    auto prov = fbu_master->getDispatcher();
    IXpbBuilder* dpb = NULL;
    ThrowStatusWrapper st(fbu_master->getStatus());
    zval rv, *database;

    try
    {
        // TODO: wtf
        database = PROP_GET(FireBird_Create_Args_ce, Create_Args, "database");

        if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
            throw Php_Firebird_Exception(zend_ce_value_error, "Database parameter not set");
            return FAILURE;
        }

        dpb = util->getXpbBuilder(&st, IXpbBuilder::DPB, NULL, 0);
        fbu_xpb_insert_object(dpb, Create_Args, ce, &fbp_database_create_zmap);

        db->att = prov->createDatabase(&st, Z_STRVAL_P(database),
            dpb->getBufferLength(&st), dpb->getBuffer(&st));

        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }

    // zval rv;
    // zval *database = OBJ_GET(ce, Create_Args, "database", &rv);

    // try
    // {
    //     dpb = util->getXpbBuilder(&st, IXpbBuilder::DPB, NULL, 0);
    //     // dpb->insertInt(&status, isc_dpb_page_size, 4 * 1024);
    //     dpb->insertString(&st, isc_dpb_user_name, "sysdba");
    //     dpb->insertString(&st, isc_dpb_password, "masterkey");

    //     db->att = prov->attachDatabase(&st, Z_STRVAL_P(database),
    //         dpb->getBufferLength(&st), dpb->getBuffer(&st));

    //     // if(fb_get_database_handle(status, &db->db_handle, db->att)){
    //     //     return status[1];
    //     // }
    //     return SUCCESS;
    // }
    // catch (...)
    // {
    //     fbu_handle_exception(&st, status);
    //     return FAILURE;
    // }
}
#endif

#if 0
ISC_INT64 fbu_get_transaction_id(ISC_STATUS* status, void *tra)
{
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        return _fbu_get_transaction_id(static_cast<ITransaction *>(tra));
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
}
#endif

#if 0
int fbu_execute_database(ISC_STATUS* status, const firebird_db *db, size_t len_sql, const char *sql, firebird_trans *tr)
{
    ITransaction *tra = NULL;
    auto att = static_cast<IAttachment*>(db->att);
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        auto tra = att->execute(&st, NULL, len_sql, sql, SQL_DIALECT_CURRENT, NULL, NULL, NULL, NULL);

        if (tra) {
            auto tr_id = _fbu_get_transaction_id(tra);
            tr->att = att;
            tr->tra = tra;
            tr->tr_id = tr_id;
            return SUCCESS;
        } else {
            if (tra) tra->release();
            return FAILURE;
        }
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        if (tra) tra->release();
        return FAILURE;
    }
}
#endif

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

#if 0
int fbu_parse_output_buffer(ISC_STATUS* status, firebird_stmt *stmt, int flags, zval *return_value)
{
    if (!stmt || !stmt->tr || !stmt->tr->tra || !stmt->out_metadata) return FAILURE;

    auto om = static_cast<IMessageMetadata*>(stmt->out_metadata);
    ThrowStatusWrapper st(fbu_master->getStatus());
    zval result;

    try
    {
        for (size_t index = 0; index < stmt->out_vars_count; index++) {
            ZVAL_UNDEF(&result);
            fbu_var_zval((IAttachment *)stmt->tr->att, (ITransaction *)stmt->tr->tra,
                om, &result, index, stmt->out_buffer, flags);

            if (flags & FBP_FETCH_INDEXED) {
                add_index_zval(return_value, index, &result);
            } else if (flags & FBP_FETCH_HASHED) {
                add_assoc_zval(return_value, om->getAlias(&st, index), &result);
            } else {
                throw Php_Firebird_Exception(zend_ce_error, "BUG: fetch method flag not set");
            }
        }
        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
}
#endif

#if 0
int fbu_fetch_next(ISC_STATUS* status, firebird_stmt *stmt)
{
    TODO("fbu_fetch_next");
    return FAILURE;
    auto curs = static_cast<IResultSet*>(stmt->curs);
    ThrowStatusWrapper st(fbu_master->getStatus());

    if (!curs) {
        php_printf("NO CURSOR!!\n");
        return FAILURE;
    }

    try
    {
        auto fetch_res = curs->fetchNext(&st, stmt->out_buffer);

        if (fetch_res == IStatus::RESULT_OK) {
            return 0;
        }

        if (fetch_res == IStatus::RESULT_NO_DATA) {
            curs->close(&st);
            curs->release();
            stmt->curs = curs = NULL;
            return 1;
        }

        return -1;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return -1;
    }
}
#endif

// int fbu_statement_bind(firebird_stmt *stmt, zval *b_vars, size_t num_bind_args)
// {
//     ThrowStatusWrapper st(fbu_master->getStatus());

//     try
//     {
//         _fbu_statement_bind(stmt, b_vars, num_bind_args);
//         return SUCCESS;
//     }
//     catch (...)
//     {
//         fbu_handle_exception(&st, status);
//         return FAILURE;
//     }
// }

int fbu_blob_get_segment(ISC_STATUS* status, firebird_blob *blob, zend_string *buf, unsigned* len)
{
    TODO("fbu_blob_get_segment");
    return FAILURE;
#if 0
    if (!blob || !blob->blo) return FAILURE;

    auto blo = static_cast<IBlob*>(blob->blo);
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        blo->getSegment(&st, ZSTR_LEN(buf), &ZSTR_VAL(buf), len);
        ZSTR_VAL(buf)[*len] = '\0';
        ZSTR_LEN(buf) = *len;

        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
#endif
}





using namespace FBP;

int fbu_database_init(zval *args, firebird_db *db)
{
    FBDEBUG("fbu_database_init(db=%p)", db);
    return fbu_call_void([&]() {
        db->dbptr = static_cast<void*>(new Database(args));
        return SUCCESS;
    });
}

int fbu_database_connect(firebird_db *db)
{
    return fbu_call_void([&]() {
        return static_cast<Database*>(db->dbptr)->connect();
    });
}

int fbu_database_create(firebird_db *db)
{
    return fbu_call_void([&]() {
        return static_cast<Database*>(db->dbptr)->create();
    });
}

int fbu_database_disconnect(firebird_db *db)
{
    return fbu_call_void([&]() {
        return static_cast<Database*>(db->dbptr)->disconnect();
    });
}

int fbu_database_drop(firebird_db *db)
{
    return fbu_call_void([&]() {
        return static_cast<Database*>(db->dbptr)->drop();
    });
}

int fbu_database_free(firebird_db *db)
{
    FBDEBUG("~%s(db=%p, att=%p)", __func__, db, db->dbptr);
    return fbu_call_void([&]() {
        delete static_cast<Database*>(db->dbptr);
        db->dbptr = nullptr;
        return SUCCESS;
    });
}

// int fbu_database_start_transaction(firebird_db *db, const firebird_tbuilder *builder, firebird_trans *tr)
// {
//     return fbu_call_void([&]() {
//         auto tra = static_cast<Database *>(db->dbptr)->start_transaction(builder);
//         tr->db = db;
//         tr->trptr = tra;
//         tr->id = tra->query_transaction_id();
//         return SUCCESS;
//     });
// }

#if 0
int fbu_database_execute(firebird_db *db, unsigned len_sql, const char *sql)
{
    return fbu_call_void([&]() {
        static_cast<Database*>(db->dbptr)->execute(len_sql, sql,
            nullptr, nullptr, nullptr, nullptr);
        return SUCCESS;
    });
}
#endif

int fbu_transaction_init(firebird_db *db, firebird_trans *tr)
{
    return fbu_call_void([&]() {
        auto dba = static_cast<Database*>(db->dbptr);
        Transaction* tra = new Transaction(dba);

        FBDEBUG("fbu_transaction_init(db=%p, db->dbptr=%p, new Transaction=%p)", db, db->dbptr, tra);

        tr->trptr = tra;
        tr->db = db;
        tr->id = 0;

        return SUCCESS;
    });
}

int fbu_transaction_start(firebird_trans *tr, const firebird_tbuilder *builder)
{
    return fbu_call_void([&]() {
        auto tra = static_cast<Transaction *>(tr->trptr);
        tra->start(builder);
        tr->id = tra->query_transaction_id();
        return SUCCESS;
    });
}

int fbu_transaction_free(firebird_trans *tr)
{
    FBDEBUG("~%s(tr->trptr=%p)", __func__, tr->trptr);

    return fbu_call_void([&]() {
        delete static_cast<Transaction *>(tr->trptr);
        tr->trptr = nullptr;
        return SUCCESS;
    });
}

int fbu_transaction_finalize(firebird_trans *tr, int mode)
{
    return fbu_call_void([&]() {
        auto tra = static_cast<Transaction *>(tr->trptr);

        if (mode == FBP_TR_COMMIT) {
            return tra->commit();
        } else if (mode == (FBP_TR_ROLLBACK | FBP_TR_RETAIN)) {
            return tra->rollback_ret();
        } else if (mode == (FBP_TR_COMMIT | FBP_TR_RETAIN)) {
            return tra->commit_ret();
        } else {
            return tra->rollback();
        }
    });
}

int fbu_transaction_execute(firebird_trans *tr, size_t len_sql, const char *sql)
{
    return fbu_call_void([&]() {
        auto tra = static_cast<Transaction *>(tr->trptr);
        auto new_tr = tra->execute(len_sql, sql);

        FBDEBUG("fbu_transaction_execute(tr->trptr=%p, new_tr=%p)", tr->trptr, new_tr);

        if (new_tr) {
            tr->id = tra->query_transaction_id();
        } else {
            // ASSUME: Rolledback / commited
            tr->id = 0;
        }

        return SUCCESS;
    });
}

int fbu_statement_init(firebird_trans *tr, firebird_stmt *stmt)
{
    return fbu_call_void([&]() {
        auto s = new Statement(static_cast<Transaction *>(tr->trptr));

        FBDEBUG("fbu_statement_init(tr=%p, new Statement=%p)", tr, s);

        stmt->sptr = s;

        return SUCCESS;
    });
}


int fbu_statement_prepare(firebird_stmt *stmt, unsigned len_sql, const char *sql)
{
    return fbu_call_void([&]() {
        auto s = static_cast<Statement *>(stmt->sptr);

        s->prepare(len_sql, sql);

        stmt->statement_type = s->statement_type;
        stmt->in_vars_count = s->in_vars_count;
        stmt->out_vars_count = s->out_vars_count;

        stmt->sql = estrdup(sql);
        stmt->sql_len = len_sql;

        stmt->is_exhausted = 0;
        stmt->is_cursor_open = 0;

        return SUCCESS;
    });
}

int fbu_statement_bind(firebird_stmt *stmt, zval *b_vars, unsigned int num_bind_args)
{
    return fbu_call_void([&]() {
        static_cast<Statement *>(stmt->sptr)->bind(b_vars, num_bind_args);
        stmt->is_exhausted = 0;
        return SUCCESS;
    });
}

int fbu_statement_open_cursor(firebird_stmt *stmt)
{
    // TODO: check if already open
    return fbu_call_void([&]() {
        static_cast<Statement *>(stmt->sptr)->open_cursor();
        stmt->is_cursor_open = 1;
        return SUCCESS;
    });
}

int fbu_statement_fetch_next(firebird_stmt *stmt)
{
    return fbu_call_void([&]() {
        auto res = static_cast<Statement *>(stmt->sptr)->fetch_next();

        if (res == IStatus::RESULT_OK) {
            return 0;
        }

        if (res == IStatus::RESULT_NO_DATA) {
            // Cursor will be closed by fetch_next()
            stmt->is_cursor_open = 0;
            stmt->is_exhausted = 1;
            return 1;
        }

        // Should do something here?

        return -1;
    });
}

HashTable *fbu_statement_output_buffer_to_array(firebird_stmt *stmt, int flags)
{
    HashTable *ht = nullptr;

    fbu_call_void([&]() {
        ht = static_cast<Statement *>(stmt->sptr)->output_buffer_to_array(flags);
        return SUCCESS;
    });

    return ht;
}

int fbu_statement_execute(firebird_stmt *stmt)
{
    return fbu_call_void([&]() {
        static_cast<Statement *>(stmt->sptr)->execute();
        return SUCCESS;
    });
}

int fbu_statement_close_cursor(firebird_stmt *stmt)
{
    return fbu_call_void([&]() {
        return static_cast<Statement *>(stmt->sptr)->close_cursor();
    });
}

void fbu_statement_free(firebird_stmt *stmt)
{
    FBDEBUG("~fbu_statement_free(%p)", stmt->sptr);

    delete static_cast<Statement *>(stmt->sptr);

    stmt->sptr = nullptr;
    if (stmt->sql) {
        efree((void *)stmt->sql);
        stmt->sql = nullptr;
    }
}

int fbu_blob_init(firebird_trans *tr, firebird_blob *blob)
{
    return fbu_call_void([&]() {
        auto blo = new Blob(static_cast<Transaction *>(tr->trptr));

        FBDEBUG("fbu_blob_init(tr=%p, tr->trptr=%p, new Blob=%p)", tr, tr->trptr, blo);

        blob->blobptr = blo;
        blob->tr = tr;
        blob->info = blo->get_info();

        return SUCCESS;
    });
}

int fbu_blob_create(firebird_blob *blob)
{
    return fbu_call_void([&]() {
        static_cast<Blob *>(blob->blobptr)->create();
        return SUCCESS;
    });
}

int fbu_blob_open(firebird_blob *blob, firebird_blob_id *blob_id)
{
    return fbu_call_void([&]() {
        static_cast<Blob *>(blob->blobptr)->open(&blob_id->bl_id);
        return SUCCESS;
    });
}

int fbu_blob_free(firebird_blob *blob)
{
    FBDEBUG("~%s(blob=%p, blob->blobptr=%p)", __func__, blob, blob->blobptr);
    return fbu_call_void([&]() {
        delete static_cast<Blob*>(blob->blobptr);
        blob->blobptr = nullptr;
        return SUCCESS;
    });
}

int fbu_blob_put(firebird_blob *blob, unsigned int buf_size, const char *buf)
{
    return fbu_call_void([&]() {
        static_cast<Blob *>(blob->blobptr)->put_contents(buf_size, buf);
        return SUCCESS;
    });
}

int fbu_blob_close(firebird_blob *blob)
{
    return fbu_call_void([&]() {
        static_cast<Blob *>(blob->blobptr)->close();
        return SUCCESS;
    });
}

int fbu_blob_cancel(firebird_blob *blob)
{
    return fbu_call_void([&]() {
        static_cast<Blob *>(blob->blobptr)->cancel();
        return SUCCESS;
    });
}

zend_string *fbu_blob_get(firebird_blob *blob, unsigned int max_len)
{
    zend_string *return_value = nullptr;
    fbu_call_void([&]() {
        return_value = static_cast<Blob *>(blob->blobptr)->get_contents(max_len);
        return SUCCESS;
    });
    return return_value;
}

int fbu_blob_seek(firebird_blob *blob, int mode, int offset, int *new_offset)
{
    return fbu_call_void([&]() {
        static_cast<Blob *>(blob->blobptr)->seek(mode, offset, new_offset);
        return SUCCESS;
    });
}

} // extern "C"

