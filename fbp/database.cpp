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

// int prepare(firebird_trans *tr, unsigned len_sql, const unsigned char *sql, firebird_stmt *stmt)
// {
//     if (!tr || !tr->trptr) return false;
//     auto tra = static_cast<Transaction *>(tr->trptr);

//     auto s = att->prepare(&st, tra, len_sql, sql, SQL_DIALECT_CURRENT,
//         IStatement::PREPARE_PREFETCH_METADATA
//     );

// }

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

} // namespace
