#include <vector>
#include <memory>
#include <firebird/Interface.h>
#include <stdint.h>

#include "firebird_utils.h"

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/transaction.hpp"

extern "C" {
#include "zend_exceptions.h"
}

using namespace Firebird;

namespace FBP {

Database::Database()
{
}

void Database::connect(zval *args)
{
    auto util = master->getUtilInterface();
    auto prov = master->getDispatcher();

    // TODO: create normal C struct
    if (!args || Z_TYPE_P(args) != IS_OBJECT || Z_OBJCE_P(args) != FireBird_Connect_Args_ce) {
        throw Php_Firebird_Exception(zend_ce_value_error, "Expected instanceof Connect_Args");
    }

    zval rv;
    zval *database = PROP_GET(FireBird_Connect_Args_ce, args, "database");
    IXpbBuilder* dpb = nullptr;

    // TODO: process other props
    dpb = util->getXpbBuilder(&st, IXpbBuilder::DPB, NULL, 0);
    // dpb->insertInt(&status, isc_dpb_page_size, 4 * 1024);
    dpb->insertString(&st, isc_dpb_user_name, "sysdba");
    dpb->insertString(&st, isc_dpb_password, "masterkey");

    att = prov->attachDatabase(&st, Z_STRVAL_P(database),
        dpb->getBufferLength(&st), dpb->getBuffer(&st));
}

void Database::create(zval *args)
{
    auto util = master->getUtilInterface();
    auto prov = master->getDispatcher();

    if (!args || Z_TYPE_P(args) != IS_OBJECT || Z_OBJCE_P(args) != FireBird_Create_Args_ce) {
        throw Php_Firebird_Exception(zend_ce_value_error, "Expected instanceof Create_Args");
    }

    zval rv;
    zval *database = PROP_GET(FireBird_Create_Args_ce, args, "database");
    IXpbBuilder* dpb = nullptr;

    dpb = util->getXpbBuilder(&st, IXpbBuilder::DPB, NULL, 0);
    fbu_xpb_insert_object(dpb, args, FireBird_Create_Args_ce, &fbp_database_create_zmap);

    dpb->insertInt(&st, isc_dpb_sql_dialect, SQL_DIALECT_CURRENT);
    // fbp_dump_buffer(dpb->getBufferLength(&st), dpb->getBuffer(&st));

    att = prov->createDatabase(&st, Z_STRVAL_P(database),
        dpb->getBufferLength(&st), dpb->getBuffer(&st));
}

void Database::disconnect()
{
    att->detach(&st);
    att->release();
    att = nullptr;
}

void Database::drop()
{
    att->dropDatabase(&st);
    att->release();
    att = nullptr;
}

Database::~Database()
{
    FBDEBUG("~Database(this=%p)", this);

    int err = 0;
    try
    {
        // trans_list.clear();
        if (att) disconnect();
    }
    catch (...)
    {
        err = 1;
    }

    if (att) {
        att->release();
        att = nullptr;
    }

    if (err) fbu_handle_exception(__FILE__, __LINE__);
}

size_t Database::transaction_init()
{
    tr_list.emplace_back(new Transaction(*this));
    return tr_list.size();
}

size_t Database::transaction_reconnect(ISC_ULONG tr_id)
{
    auto tr = new Transaction(*this);
    tr->set(att->reconnectTransaction(&st, sizeof(tr_id), (const unsigned char*)&tr_id));
    tr->query_info();

    tr_list.emplace_back(tr);
    return tr_list.size();
}

void Database::transaction_free(size_t trh)
{
    get_transaction(trh).reset();
}

std::unique_ptr<Transaction> &Database::get_transaction(size_t trh)
{
    FBDEBUG("Database::get_transaction(trh=%lu, tr_list.size=%lu)", trh, tr_list.size());
    if (!trh || trh > tr_list.size() || !tr_list[trh - 1]) {
        throw Php_Firebird_Exception(zend_ce_error, "Invalid transaction handle");
    }

    return tr_list[trh - 1];
}

size_t Database::statement_init(size_t trh)
{
    st_list.emplace_back(new Statement(*get_transaction(trh)));
    return st_list.size();
}

size_t Database::blob_init(size_t trh)
{
    bl_list.emplace_back(new Blob(*get_transaction(trh)));
    return bl_list.size();
}

void Database::statement_free(size_t sth)
{
    get_statement(sth).reset();
}

std::unique_ptr<Statement> &Database::get_statement(size_t sth)
{
    if (!sth || sth > st_list.size() || !st_list[sth - 1]) {
        throw Php_Firebird_Exception(zend_ce_error, "Invalid statement handle");
    }

    return st_list[sth - 1];
}

std::unique_ptr<Blob> &Database::get_blob(size_t blh)
{
    if (!blh || blh > bl_list.size() || !bl_list[blh - 1]) {
        throw Php_Firebird_Exception(zend_ce_error, "Invalid blob handle");
    }

    return bl_list[blh - 1];
}

void Database::blob_free(size_t blh)
{
    get_blob(blh).reset();
}

IAttachment *Database::get()
{
    FBDEBUG("Database::get()=%p", att);
    if (att) {
        return att;
    }
    throw Php_Firebird_Exception(zend_ce_error, "Invalid database pointer");
}

void Database::execute_create(unsigned int len_sql, const char *sql)
{
    if (att) {
        throw Php_Firebird_Exception(zend_ce_value_error, "Database already initialized");
    }

    auto util = master->getUtilInterface();
    // executeCreateDatabase(StatusType* status, unsigned stmtLength, const char* creatDBstatement, unsigned dialect, FB_BOOLEAN* stmtIsCreateDb)
    att = util->executeCreateDatabase(&st, len_sql, sql, SQL_DIALECT_CURRENT, NULL);
}

#define READ_BIGINT(name)                      \
    zend_update_property_long(                 \
        FireBird_Db_Info_ce, Z_OBJ_P(db_info), \
        name, sizeof(name) - 1, b->getBigInt(&st))

#define READ_STRING(name)                      \
    zend_update_property_stringl(              \
        FireBird_Db_Info_ce, Z_OBJ_P(db_info), \
        name, sizeof(name) - 1, (const char *)b->getBytes(&st), len)

#define READ_STRING_ARRAY(name)                                 \
{                                                               \
    auto buf = b->getBytes(&st);                                \
    ISC_UCHAR count = *buf++;                                   \
    ISC_UCHAR str_len;                                          \
    zval str_arr;                                               \
    array_init(&str_arr);                                       \
    for (int i = 0; i < count; i++) {                           \
        str_len = *buf++;                                       \
        add_next_index_stringl(&str_arr, (char *)buf, str_len); \
        buf += str_len;                                         \
    }                                                           \
    zend_update_property(                                       \
        FireBird_Db_Info_ce, Z_OBJ_P(db_info),                  \
        name, sizeof(name) - 1, &str_arr);                      \
    zval_ptr_dtor(&str_arr);                                    \
}

void Database::get_info(zval *db_info)
{
    // firebird\src\jrd\inf.cpp
    const unsigned char req[] = {
        // isc_info_db_id              Not very useful. Path to DB, HOST, sizable byte array, needs parsing
        isc_info_reads,
        isc_info_writes,
        isc_info_fetches,
        isc_info_marks,

        // isc_info_implementation      // Bunch of codes from info_db_implementations, needs parsing
        isc_info_isc_version,

        // isc_info_base_level         This info item is so old it apparently uses an archaic format, not a standard vax integer format
        isc_info_page_size,
        isc_info_num_buffers,
        // isc_info_limbo,
        isc_info_current_memory,
        isc_info_max_memory,
        // isc_info_window_turns       Unused
        // isc_info_license            Unused

        isc_info_allocation,
        isc_info_attachment_id,

        // These are per table?
        // isc_info_read_seq_count,
        // isc_info_read_idx_count,
        // isc_info_insert_count,
        // isc_info_update_count,
        // isc_info_delete_count,
        // isc_info_backout_count,
        // isc_info_purge_count,
        // isc_info_expunge_count,

        isc_info_sweep_interval,
        isc_info_ods_version,
        isc_info_ods_minor_version,
        isc_info_no_reserve,

        // Some deprecated WAL and JOURNAL items was skipped here

        isc_info_forced_writes,
        // isc_info_user_names list of connected user. isql queries mon$attachments when major_ods >= ODS_VERSION12

        // Maybe in info class some day
        // isc_info_page_errors = 54,
        // isc_info_record_errors = 55,
        // isc_info_bpage_errors = 56,
        // isc_info_dpage_errors = 57,
        // isc_info_ipage_errors = 58,
        // isc_info_ppage_errors = 59,
        // isc_info_tpage_errors = 60,

        isc_info_set_page_buffers,
        isc_info_db_sql_dialect,
        isc_info_db_read_only,
        isc_info_db_size_in_pages,

        // frb_info_att_charset                 Charset ID?
        // isc_info_db_class                    Legacy. Returns isc_info_db_class_classic_access or isc_info_db_class_server_access
        isc_info_firebird_version,

        isc_info_oldest_transaction,
        isc_info_oldest_active,
        isc_info_oldest_snapshot,
        isc_info_next_transaction,

        // isc_info_db_provider                 Legacy. Returns just isc_info_db_code_firebird
        // isc_info_active_transactions         List of active transactions
        // isc_info_active_tran_count           Same loops isc_info_active_transactions but just retuns count

        isc_info_creation_date,
        isc_info_db_file_size,
        // fb_info_page_contents                Reads raw pages?

        // fb_info_implementation               Bunch of codes from info_db_implementations, needs parsing

        // Maybe in info class some day
        // fb_info_page_warns = 115,
        // fb_info_record_warns = 116,
        // fb_info_bpage_warns = 117,
        // fb_info_dpage_warns = 118,
        // fb_info_ipage_warns = 119,
        // fb_info_ppage_warns = 120,
        // fb_info_tpage_warns = 121,
        // fb_info_pip_errors = 122,
        // fb_info_pip_warns = 123,

        fb_info_pages_used,
        fb_info_pages_free,

        fb_info_ses_idle_timeout_db,
        fb_info_ses_idle_timeout_att,
        fb_info_ses_idle_timeout_run,
        fb_info_conn_flags,

        fb_info_crypt_key,
        fb_info_crypt_state,

        fb_info_statement_timeout_db,
        fb_info_statement_timeout_att,

        fb_info_protocol_version,
        fb_info_crypt_plugin,

        // fb_info_creation_timestamp_tz = 139,

        fb_info_wire_crypt,

        // fb_info_features                          Results of info_features. All features filled on FB5.0. Test on older versions

        fb_info_next_attachment,
        fb_info_next_statement,
        fb_info_db_guid,
        // fb_info_db_file_id,                       // some kind of sys file stat

        fb_info_replica_mode,                        // enum replica_mode

        fb_info_username,
        fb_info_sqlrole,

        fb_info_parallel_workers,

        // Wire stats items, implemented by Remote provider only
        // fb_info_wire_out_packets = 150,
        // fb_info_wire_in_packets = 151,
        // fb_info_wire_out_bytes = 152,
        // fb_info_wire_in_bytes = 153,
        // fb_info_wire_snd_packets = 154,
        // fb_info_wire_rcv_packets = 155,
        // fb_info_wire_snd_bytes = 156,
        // fb_info_wire_rcv_bytes = 157,
        // fb_info_wire_roundtrips = 158,
    };

    unsigned char resp[1024] = { 0 };
    att->getInfo(&st, sizeof(req), req, sizeof(resp), resp);
    auto b = master->getUtilInterface()->getXpbBuilder(&st, IXpbBuilder::INFO_RESPONSE, resp, sizeof(resp));

    fbp_dump_buffer(300, b->getBuffer(&st));

    int pos = 0;
    for (b->rewind(&st); !b->isEof(&st); b->moveNext(&st)) {
        auto tag = b->getTag(&st);
        auto len = b->getLength(&st);

        php_printf("tag=%d / 0x%x, pos=%d\n", tag, tag, pos);

        if (tag == isc_info_end) break;

        switch(tag) {
            case isc_info_isc_version     : READ_STRING_ARRAY("isc_version"); break;
            case isc_info_firebird_version: READ_STRING_ARRAY("firebird_version"); break;

            case isc_info_creation_date: {
                struct tm t;
                isc_decode_timestamp((ISC_TIMESTAMP *) b->getBytes(&st), &t);
                zend_update_property_long(FireBird_Db_Info_ce, Z_OBJ_P(db_info), "creation_date", sizeof("creation_date") - 1, mktime(&t));
            } break;

            // case isc_info_limbo: {
            //     if(limbo_count++ < max_limbo_count) {
            //         add_next_index_long(&info_limbo, IXpbBuilder_getBigInt(dpb, st));
            //     } else {
            //         IXpbBuilder_moveNext(dpb, st);
            //     }
            // } break;

            // TODO: parse per table stats
            // case isc_info_read_idx_count   : READ_BIGINT("read_idx_count"); break;
            // case isc_info_insert_count     : READ_BIGINT("insert_count"); break;
            // case isc_info_update_count     : READ_BIGINT("update_count"); break;
            // case isc_info_delete_count     : READ_BIGINT("delete_count"); break;
            // case isc_info_backout_count    : READ_BIGINT("backout_count"); break;
            // case isc_info_purge_count      : READ_BIGINT("purge_count"); break;
            // case isc_info_expunge_count    : READ_BIGINT("expunge_count"); break;

            case isc_info_reads               : READ_BIGINT("reads"); break;
            case isc_info_writes              : READ_BIGINT("writes"); break;
            case isc_info_fetches             : READ_BIGINT("fetches"); break;
            case isc_info_marks               : READ_BIGINT("marks"); break;
            case isc_info_page_size           : READ_BIGINT("page_size"); break;
            case isc_info_num_buffers         : READ_BIGINT("num_buffers"); break;
            case isc_info_current_memory      : READ_BIGINT("current_memory"); break;
            case isc_info_max_memory          : READ_BIGINT("max_memory"); break;
            case isc_info_allocation          : READ_BIGINT("allocation"); break;
            case isc_info_attachment_id       : READ_BIGINT("attachment_id"); break;
            case isc_info_sweep_interval      : READ_BIGINT("sweep_interval"); break;
            case isc_info_ods_version         : READ_BIGINT("ods_version"); break;
            case isc_info_ods_minor_version   : READ_BIGINT("ods_minor_version"); break;
            case isc_info_no_reserve          : READ_BIGINT("no_reserve"); break;
            case isc_info_forced_writes       : READ_BIGINT("forced_writes"); break;
            case isc_info_set_page_buffers    : READ_BIGINT("set_page_buffers"); break;
            case isc_info_db_sql_dialect      : READ_BIGINT("db_sql_dialect"); break;
            case isc_info_db_read_only        : READ_BIGINT("db_read_only"); break;
            case isc_info_db_size_in_pages    : READ_BIGINT("db_size_in_pages"); break;
            case isc_info_oldest_transaction  : READ_BIGINT("oldest_transaction"); break;
            case isc_info_oldest_active       : READ_BIGINT("oldest_active"); break;
            case isc_info_oldest_snapshot     : READ_BIGINT("oldest_snapshot"); break;
            case isc_info_next_transaction    : READ_BIGINT("next_transaction"); break;
            case isc_info_db_file_size        : READ_BIGINT("db_file_size"); // always 0 for me break;
            case fb_info_pages_used           : READ_BIGINT("pages_used"); break;
            case fb_info_pages_free           : READ_BIGINT("pages_free"); break;
            case fb_info_ses_idle_timeout_db  : READ_BIGINT("ses_idle_timeout_db"); break;
            case fb_info_ses_idle_timeout_att : READ_BIGINT("ses_idle_timeout_att"); break;
            case fb_info_ses_idle_timeout_run : READ_BIGINT("ses_idle_timeout_run"); break;
            case fb_info_conn_flags           : READ_BIGINT("conn_flags"); break;
            case fb_info_crypt_state          : READ_BIGINT("crypt_state"); break;
            case fb_info_statement_timeout_db : READ_BIGINT("statement_timeout_db"); break;
            case fb_info_statement_timeout_att: READ_BIGINT("statement_timeout_att"); break;
            case fb_info_protocol_version     : READ_BIGINT("protocol_version"); break;
            case fb_info_crypt_key            : READ_STRING("crypt_key"); break;
            case fb_info_crypt_plugin         : READ_STRING("crypt_plugin"); break;
            case fb_info_wire_crypt           : READ_STRING("wire_crypt"); break;
            case fb_info_next_attachment      : READ_BIGINT("next_attachment"); break;
            case fb_info_next_statement       : READ_BIGINT("next_statement"); break;
            case fb_info_db_guid              : READ_STRING("db_guid"); break;
            case fb_info_replica_mode         : READ_BIGINT("replica_mode"); break;
            case fb_info_username             : READ_STRING("username"); break;
            case fb_info_sqlrole              : READ_STRING("sqlrole"); break;
            case fb_info_parallel_workers     : READ_BIGINT("parallel_workers"); break;

            default:
                fbp_fatal("BUG! Unhandled DB info tag: %d at pos: %d", tag, pos);
                break;
        }
        pos += 3 + b->getLength(&st);
    }
}

void Database::get_limbo_transactions(zval *tr_arr, short max_count)
{
    const unsigned char req[] = { isc_info_limbo };

    size_t resp_capacity = TRANS_ID_SIZE * max_count;
    auto resp = (unsigned char *)emalloc(resp_capacity);

    att->getInfo(&st, sizeof(req), req, resp_capacity, resp);
    auto b = master->getUtilInterface()->getXpbBuilder(&st, IXpbBuilder::INFO_RESPONSE, resp, resp_capacity);

    size_t limbo_count = 0;
    for (b->rewind(&st); !b->isEof(&st); b->moveNext(&st)) {
        auto tag = b->getTag(&st);

        if (tag == isc_info_end) break;

        switch(tag) {
            case isc_info_limbo: {
                if(limbo_count++ < max_count) {
                    add_next_index_long(tr_arr, b->getBigInt(&st));
                } else {
                    goto finish_with_limbo;
                }
            } break;

            default:
                fbp_fatal("BUG! Unhandled DB info tag: %d", tag);
                break;
        }
    }
finish_with_limbo:
    efree(resp);
}

} // namespace
