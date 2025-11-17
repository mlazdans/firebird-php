/*
  +----------------------------------------------------------------------+
  | Authors: Martins Lazdans <marrtins@dqdp.net>                         |
  +----------------------------------------------------------------------+
*/

#include <cstring>
#include <stdexcept>
#include <sstream>
#include <firebird/Interface.h>
#include "firebird_utils.h"

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

class Php_Firebird_Exception : public std::runtime_error {
public:
    zend_class_entry *ce;
    explicit Php_Firebird_Exception(zend_class_entry *ce, const std::string& msg)
        : std::runtime_error(msg),
        ce(ce)
        {}
};

static void fbu_handle_exception(Firebird::ThrowStatusWrapper *st, ISC_STATUS* status)
{
    try
    {
        throw;
    }
    catch (const FbException& e)
    {
        fbu_copy_status((const ISC_STATUS*)e.getStatus()->getErrors(), status, 20);

        // IMaster* fbu_master = fb_get_master_interface();
        // IUtil* fb_util = fbu_master->getUtilInterface();
        // ISC_STATUS* vector = e.getStatus()->getErrors();
        // const ISC_STATUS* vector = e.getStatus()->getErrors();
        // vector contains full Firebird status
        // char msg[1024];
        // fb_util->formatStatus(msg, sizeof(msg), e.getStatus());
        // fprintf(stderr, "Error: %s\n", msg);

        // fbu_copy_status((const ISC_STATUS*)st->getErrors(), status, 20);
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

const char* fbu_get_sql_type_name(unsigned type)
{
    switch (type)
    {
    case SQL_TEXT:            return "TEXT";
    case SQL_VARYING:         return "VARYING";
    case SQL_SHORT:           return "SHORT";
    case SQL_LONG:            return "LONG";
    case SQL_INT64:           return "INT64";
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "ibase.h"

#include "zend_exceptions.h"
#include "ext/date/php_date.h"

#include "database.h"
#include "fbp_database.h"
#include "fbp_transaction.h"
#include "fbp_statement.h"
#include "statement.h"
#include "fbp_blob.h"

IMaster* fbu_master = fb_get_master_interface();

#define STRNUM_PARSE_OK       0
#define STRNUM_PARSE_ERROR    1
#define STRNUM_PARSE_OVERFLOW 2

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

int fbu_attach_database(ISC_STATUS* status, firebird_db *db, zval *Connect_Args, zend_class_entry *ce)
{
    auto util = fbu_master->getUtilInterface();
    auto prov = fbu_master->getDispatcher();
    IXpbBuilder* dpb = NULL;
    ThrowStatusWrapper st(fbu_master->getStatus());

    zval rv;
    zval *database = OBJ_GET(ce, Connect_Args, "database", &rv);

    try
    {
        dpb = util->getXpbBuilder(&st, IXpbBuilder::DPB, NULL, 0);
        // dpb->insertInt(&status, isc_dpb_page_size, 4 * 1024);
        dpb->insertString(&st, isc_dpb_user_name, "sysdba");
        dpb->insertString(&st, isc_dpb_password, "masterkey");

        db->att = prov->attachDatabase(&st, Z_STRVAL_P(database),
            dpb->getBufferLength(&st), dpb->getBuffer(&st));

        // if(fb_get_database_handle(status, &db->db_handle, db->att)){
        //     return status[1];
        // }
        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
}

int fbu_detach_database(ISC_STATUS* status, firebird_db *db)
{
    IXpbBuilder* dpb = NULL;
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        static_cast<IAttachment*>(db->att)->detach(&st);
        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
}

void fbu_transaction_build_tpb(IXpbBuilder *tpb, firebird_tbuilder *builder)
{
    // auto util = fbu_master->getUtilInterface();
    // IXpbBuilder* tpb = NULL;
    ThrowStatusWrapper st(fbu_master->getStatus());

    FBDEBUG("Creating transaction start buffer");

    if (!builder) return;

    // tpb = util->getXpbBuilder(&st, IXpbBuilder::TPB, NULL, 0);
    // tpb->insertTag(&st, isc_tpb_read_committed);
    // tpb->insertTag(&st, isc_tpb_no_rec_version);
    // tpb->insertTag(&st, isc_tpb_wait);
    // tpb->insertTag(&st, isc_tpb_read);
    // tra = att->startTransaction(&status, tpb->getBufferLength(&status), tpb->getBuffer(&status));

    // *p++ = isc_tpb_version3;

    tpb->insertTag(&st, builder->read_only ? isc_tpb_read : isc_tpb_write);

    if (builder->ignore_limbo) tpb->insertTag(&st, isc_tpb_ignore_limbo);
    if (builder->auto_commit) tpb->insertTag(&st, isc_tpb_autocommit);
    if (builder->no_auto_undo) tpb->insertTag(&st, isc_tpb_no_auto_undo);

    if (builder->isolation_mode == 0) {
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_consistency");
        tpb->insertTag(&st, isc_tpb_consistency);
    } else if (builder->isolation_mode == 1) {
        tpb->insertTag(&st, isc_tpb_concurrency);
        if (builder->snapshot_at_number) {
            tpb->insertInt(&st, isc_tpb_at_snapshot_number, builder->snapshot_at_number);
            // *p++ = isc_tpb_at_snapshot_number;
            // *p++ = sizeof(builder->snapshot_at_number);
            // fbp_store_portable_integer(p, builder->snapshot_at_number, sizeof(builder->snapshot_at_number));
            // p += sizeof(builder->snapshot_at_number);
            FBDEBUG_NOFL("  isolation_mode = isc_tpb_concurrency");
            FBDEBUG_NOFL("                   isc_tpb_at_snapshot_number = %d", builder->snapshot_at_number);
        } else {
            FBDEBUG_NOFL("  isolation_mode = isc_tpb_concurrency");
        }
    } else if (builder->isolation_mode == 2) {
        tpb->insertTag(&st, isc_tpb_read_committed);
        tpb->insertTag(&st, isc_tpb_rec_version);
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_rec_version");
    } else if (builder->isolation_mode == 3) {
        tpb->insertTag(&st, isc_tpb_read_committed);
        tpb->insertTag(&st, isc_tpb_no_rec_version);
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_no_rec_version");
    } else if (builder->isolation_mode == 4) {
        tpb->insertTag(&st, isc_tpb_read_committed);
        tpb->insertTag(&st, isc_tpb_read_consistency);
        FBDEBUG_NOFL("  isolation_mode = isc_tpb_read_committed");
        FBDEBUG_NOFL("                   isc_tpb_read_consistency");
    } else {
        fbp_fatal("BUG! unknown transaction isolation_mode: %d", builder->isolation_mode);
    }

    if (builder->lock_timeout == 0) {
        tpb->insertTag(&st, isc_tpb_nowait);
        FBDEBUG_NOFL("  isc_tpb_nowait");
    } else if (builder->lock_timeout == -1) {
        tpb->insertTag(&st, isc_tpb_wait);
        FBDEBUG_NOFL("  isc_tpb_wait");
    } else if (builder->lock_timeout > 0) {
        tpb->insertTag(&st, isc_tpb_wait);
        tpb->insertInt(&st, isc_tpb_lock_timeout, builder->lock_timeout);
        FBDEBUG_NOFL("  isc_tpb_wait");
        FBDEBUG_NOFL("    isc_tpb_lock_timeout = %d", builder->lock_timeout);
    } else {
        fbp_fatal("BUG! invalid lock_timeout: %d", builder->lock_timeout);
    }
}

int fbu_start_transaction(ISC_STATUS* status, firebird_trans *tr)
{
    // if (!tr || !tr->att) return FAILURE;

    auto util = fbu_master->getUtilInterface();
    ThrowStatusWrapper st(fbu_master->getStatus());
    IXpbBuilder* tpb = util->getXpbBuilder(&st, IXpbBuilder::TPB, NULL, 0);

    try
    {
        fbu_transaction_build_tpb(tpb, tr->builder);
        auto tra = static_cast<IAttachment *>(tr->att)->startTransaction(&st, tpb->getBufferLength(&st), tpb->getBuffer(&st));

        unsigned char req[] = { isc_info_tra_id };
        unsigned char resp[16];
        tra->getInfo(&st, sizeof(req), req, sizeof(resp), resp);

        IXpbBuilder* dpb = util->getXpbBuilder(&st, IXpbBuilder::INFO_RESPONSE, resp, sizeof(resp));

        for (dpb->rewind(&st); !dpb->isEof(&st); dpb->moveNext(&st)) {
            auto tag = dpb->getTag(&st);
            if (tag == isc_info_tra_id) {
                tr->tr_id = dpb->getBigInt(&st);
                break;
            }
        }

        tr->tra = tra;

        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
}

int fbu_finalize_transaction(ISC_STATUS* status, firebird_trans *tr, int mode)
{
    if (!tr) return FAILURE;

    ThrowStatusWrapper st(fbu_master->getStatus());
    auto tra = static_cast<ITransaction*>(tr->tra);

    try {
        if (mode == FBP_TR_COMMIT) {
            tra->commit(&st);
            tr->tra = tra = NULL;
        } else if (mode == (FBP_TR_ROLLBACK | FBP_TR_RETAIN)) {
            tra->rollbackRetaining(&st);
        } else if (mode == (FBP_TR_COMMIT | FBP_TR_RETAIN)) {
            tra->commitRetaining(&st);
        } else {
            tra->rollback(&st);
            tr->tra = tra = NULL;
        }
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }

    return SUCCESS;
}

int fbu_prepare_statement(ISC_STATUS* status, firebird_stmt *stmt, const char *sql)
{
    auto att = static_cast<IAttachment*>(stmt->tr->att);
    auto tra = static_cast<ITransaction*>(stmt->tr->tra);
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        auto s = att->prepare(&st, tra, 0, sql, SQL_DIALECT_CURRENT,
            IStatement::PREPARE_PREFETCH_METADATA
        );

        auto in_metadata = s->getInputMetadata(&st);
        auto out_metadata = s->getOutputMetadata(&st);

        stmt->statement_type = s->getType(&st);

        stmt->in_vars_count = in_metadata->getCount(&st);
        stmt->in_buffer_len = in_metadata->getMessageLength(&st);
        stmt->in_buffer = (unsigned char *)ecalloc(stmt->in_buffer_len, sizeof(*stmt->in_buffer));

        stmt->out_vars_count = out_metadata->getCount(&st);
        // TODO: alloc just before fetch
        stmt->out_buffer_len = out_metadata->getMessageLength(&st);
        stmt->out_buffer = (unsigned char *)ecalloc(stmt->out_buffer_len, sizeof(*stmt->out_buffer));

        stmt->in_metadata = in_metadata;
        stmt->out_metadata = out_metadata;
        stmt->stmt = s;

        // stmt->has_more_rows = stmt->out_vars_count > 0;

        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
}

int fbu_free_statement(ISC_STATUS* status, firebird_stmt *stmt)
{
    php_printf("fbu_free_statement!!\n");

    auto s = static_cast<IStatement*>(stmt->stmt);
    auto curs = static_cast<IResultSet*>(stmt->curs);
    auto im = static_cast<IMessageMetadata*>(stmt->in_metadata);
    auto om = static_cast<IMessageMetadata*>(stmt->out_metadata);

    int rv;

    ThrowStatusWrapper st(fbu_master->getStatus());

    try {
        if (curs) {
            curs->close(&st);
            curs = NULL;
        }

        if (s) {
            s->free(&st);
            s = NULL;
        }
        rv = SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        rv = FAILURE;
    }

    if (im) im->release();
    if (om) om->release();
    if (curs) curs->release();
    if (s) s->release();

    if (stmt->in_buffer) {
        efree(stmt->in_buffer);
        stmt->in_buffer = NULL;
    }

    if (stmt->out_buffer) {
        efree(stmt->out_buffer);
        stmt->out_buffer = NULL;
    }

    return rv;
}

int fbu_execute_statement(ISC_STATUS* status, firebird_stmt *stmt)
{
    auto tra = static_cast<ITransaction*>(stmt->tr->tra);
    auto s = static_cast<IStatement*>(stmt->stmt);
    auto im = static_cast<IMessageMetadata*>(stmt->in_metadata);
    auto om = static_cast<IMessageMetadata*>(stmt->out_metadata);
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        s->execute(&st, tra, im, stmt->in_buffer, om, stmt->out_buffer);
        // if (stmt->in_vars_count && stmt->out_vars_count) {
        //     s->execute(&st, tra, im, stmt->in_buffer, om, stmt->out_buffer);
        // } else if (stmt->in_vars_count) {
        //     s->execute(&st, tra, im, stmt->in_buffer, NULL, NULL);
        // } else if (stmt->out_vars_count) {
        //     s->execute(&st, tra, NULL, NULL, om, stmt->out_buffer);
        // } else {
        //     s->execute(&st, tra, NULL, NULL, NULL, NULL);
        // }

        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
}

int fbu_open_cursor(ISC_STATUS* status, firebird_stmt *stmt)
{
    auto tra = static_cast<ITransaction*>(stmt->tr->tra);
    auto s = static_cast<IStatement*>(stmt->stmt);
    auto im = static_cast<IMessageMetadata*>(stmt->in_metadata);
    auto om = static_cast<IMessageMetadata*>(stmt->out_metadata);
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        // s->openCursor(&st, tra, NULL, NULL, NULL, NULL, 0);
        // openCursor(StatusType* status, ITransaction* transaction, IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata, unsigned flags)
        auto curs = s->openCursor(&st, tra, im, stmt->in_buffer, om, 0);

        // curs->addRef();
        stmt->curs = curs;

        // allocate output buffer
        // unsigned l = om->getMessageLength(&status);
        // unsigned char* buffer = new unsigned char[l];

        // s->fet(&st, tra, NULL, NULL, NULL, NULL);
        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
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

static int fbu_var_zval(zval *val, size_t index, IMessageMetadata *om, unsigned char* buffer, int flags)
{
    IUtil* util = fbu_master->getUtilInterface();
    ThrowStatusWrapper st(fbu_master->getStatus());

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

    auto type = om->getType(&st, index);
    auto len = om->getLength(&st, index);
    auto scale = om->getScale(&st, index);
    auto data = buffer + om->getOffset(&st, index);

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
            fbp_fatal("unhandled type: %d, field:%s", type, om->getAlias(&st, index));

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
            // TODO
            break;

#if 0
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
format_date_time:
            /*
              XXX - Might have to remove this later - seems that isc_decode_date()
               always sets tm_isdst to 0, sometimes incorrectly (InterBase 6 bug?)
            */
            t.tm_isdst = -1;
#if HAVE_STRUCT_TM_TM_ZONE
            t.tm_zone = tzname[0];
#endif
            if ((type != SQL_TYPE_TIME) && (flags & FBP_FETCH_UNIXTIME)) {
                ZVAL_LONG(val, mktime(&t));
            } else {
                l = strftime(string_data, sizeof(string_data), format, &t);
                // ZVAL_NULL(val);
                ZVAL_STRINGL(val, string_data, l);
                break;
            }
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

int fbu_fetch(ISC_STATUS* status, firebird_stmt *stmt, int flags, zval *return_value)
{
    auto s = static_cast<IStatement*>(stmt->stmt);
    auto om = static_cast<IMessageMetadata*>(stmt->out_metadata);
    auto curs = static_cast<IResultSet*>(stmt->curs);
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        zval result;
        auto cols = om->getCount(&st);

        if (curs->isEof(&st)) {
            return 1;
        }

        auto fetch_res = curs->fetchNext(&st, stmt->out_buffer);

        if (fetch_res == IStatus::RESULT_OK) {
            for (size_t index = 0; index < cols; index++) {
                ZVAL_UNDEF(&result);
                fbu_var_zval(&result, index, om, stmt->out_buffer, flags);
                if (flags & FBP_FETCH_INDEXED) {
                    add_index_zval(return_value, index, &result);
                } else if (flags & FBP_FETCH_HASHED) {
                    add_assoc_zval(return_value, om->getAlias(&st, index), &result);
                } else {
                    throw Php_Firebird_Exception(zend_ce_error, "BUG: fetch method flag not set");
                }
            }
            return 0;
        }

        return -1;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return -1;
    }
}

int _fbu_statement_bind(firebird_stmt *stmt, zval *b_vars, size_t num_bind_args)
{
    auto util = fbu_master->getUtilInterface();
    auto s = static_cast<IStatement*>(stmt->stmt);
    auto im = static_cast<IMessageMetadata*>(stmt->in_metadata);
    auto curs = static_cast<IResultSet*>(stmt->curs);
    char tmp_buf[256];
    struct tm t;
    const char *tformat;
    int64_t long_min, long_max;
    zend_long long_val;

    ThrowStatusWrapper st(fbu_master->getStatus());

    if (num_bind_args != stmt->in_vars_count) {
        slprintf(tmp_buf, sizeof(tmp_buf), "Statement expects %d arguments, %d given",
            stmt->in_vars_count, num_bind_args);

        throw Php_Firebird_Exception(zend_ce_argument_count_error, std::string(tmp_buf));
    }

    auto buffer = stmt->in_buffer;

    for (size_t i = 0; i < stmt->in_vars_count; ++i) {
        zval *b_var = &b_vars[i];

        auto sqldata = buffer + im->getOffset(&st, i);
        auto scale = im->getScale(&st, i);

        // XSQLVAR *var = &sqlda->sqlvar[i];
        // auto *sqlind = &stmt->bind_buf[i].sqlind;

        auto sqlind = reinterpret_cast<short*>(buffer + im->getNullOffset(&st, i));
        // short *sqlind = (short*)&buffer[im->getNullOffset(&st, i)];

        if (Z_TYPE_P(b_var) == IS_NULL) {
            *sqlind = -1;
            // stmt->bind_buf[i].sqlind = -1;
            // sqldata = NULL;
            continue;
        }

        *sqlind = 0;
        // stmt->bind_buf[i].sqlind = 0;
        // sqldata = (unsigned char *)&stmt->bind_buf[i].val;

        auto sqltype = im->getType(&st, i);

        php_printf(
            "sql type: %s, php type: %s\n"
            "-----------------------------\n",
            fbu_get_sql_type_name(sqltype), zend_zval_type_name(b_var));

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

                if (Z_TYPE_P(b_var) == IS_STRING) {
                    uint64_t res;
                    int sign, exp;

                    php_printf("type: %s; scale: %d; s: %s\n",
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

                    php_printf("sign: %d; exp: %d; res: %zu; final val: %ld\n",
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
                TODO("SQL_BLOB");
                // ISC_QUAD bl_id;
                // if (Z_TYPE_P(b_var) == IS_OBJECT && Z_OBJCE_P(b_var) == FireBird_Blob_Id_ce) {
                //     bl_id = get_firebird_blob_id_from_zval(b_var)->bl_id;
                // } else if (Z_TYPE_P(b_var) == IS_OBJECT && Z_OBJCE_P(b_var) == FireBird_Blob_ce) {
                //     bl_id = get_firebird_blob_from_zval(b_var)->bl_id;
                // } else {
                //     convert_to_string(b_var);

                //     firebird_blob blob = {0};
                //     fbp_blob_ctor(&blob, stmt->db_handle, stmt->tr_handle);

                //     if (fbp_blob_create(&blob) ||
                //         fbp_blob_put(&blob, Z_STRVAL_P(b_var), Z_STRLEN_P(b_var)) ||
                //         fbp_blob_close(&blob)) {
                //             return FAILURE;
                //     }

                //     bl_id = blob.bl_id;
                // }
                // stmt->bind_buf[i].val.qval = bl_id;
                // *reinterpret_cast<ISC_QUAD*>(sqldata) = static_cast<ISC_QUAD>(bl_id);
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
    }
    } // for

    return SUCCESS;
}

int fbu_statement_bind(ISC_STATUS* status, firebird_stmt *stmt, zval *b_vars, size_t num_bind_args)
{
    ThrowStatusWrapper st(fbu_master->getStatus());

    try
    {
        _fbu_statement_bind(stmt, b_vars, num_bind_args);
        return SUCCESS;
    }
    catch (...)
    {
        fbu_handle_exception(&st, status);
        return FAILURE;
    }
}

#ifdef __cplusplus
}
#endif
