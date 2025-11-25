#include <vector>
#include <memory>
#include <firebird/Interface.h>
#include <stdint.h>

#include "firebird_php.hpp"
#include "firebird_utils.h"

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/transaction.hpp"

extern "C" {
// #include "php.h"
#include "zend_exceptions.h"
// #include "zend_attributes.h"
// #include "php_firebird_includes.h"
// #include "ext/spl/spl_exceptions.h"
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

ITransaction *Database::execute(ITransaction *tra, unsigned len_sql, const char *sql,
    IMessageMetadata* im, unsigned char *in_buffer,
    IMessageMetadata* om, unsigned char *out_buffer)
{
    return att->execute(&st, tra, len_sql, sql, SQL_DIALECT_CURRENT, im, in_buffer, om, out_buffer);
}

IBlob *Database::open_blob(ITransaction *transaction, ISC_QUAD *blob_id)
{
    const unsigned char bpb[] = {
        isc_bpb_version1,
        isc_bpb_type, 1, isc_bpb_type_stream
    };

    return att->openBlob(&st, transaction, blob_id, sizeof(bpb), bpb);
}

IBlob *Database::create_blob(ITransaction *transaction, ISC_QUAD *blob_id)
{
    const unsigned char bpb[] = {
        isc_bpb_version1,
        isc_bpb_type, 1, isc_bpb_type_stream
    };

    return att->createBlob(&st, transaction, blob_id, sizeof(bpb), bpb);
}

IStatement *Database::prepare(ITransaction *transaction, unsigned int len_sql, const char *sql)
{
    return att->prepare(&st, transaction, len_sql, sql, SQL_DIALECT_CURRENT,
        IStatement::PREPARE_PREFETCH_METADATA);
}

ITransaction* Database::start_transaction(unsigned int tpb_len, const unsigned char* tpb)
{
    return att->startTransaction(&st, tpb_len, tpb);
}

size_t Database::new_transaction()
{
    FBDEBUG("Database::new_transaction(this=%p)", PTR(*this));
    tr_list.emplace_back(new Transaction(*this));
    return tr_list.size();
}

std::unique_ptr<Transaction> &Database::get_transaction(size_t trh)
{
    FBDEBUG("Database::get_transaction(trh=%lu, tr_list.size=%lu)", trh, tr_list.size());
    if (!trh || trh > tr_list.size() || !tr_list[trh - 1]) {
        throw Php_Firebird_Exception(zend_ce_error, "Invalid transaction handle");
    }

    return tr_list[trh - 1];
}

std::unique_ptr<Statement> &Database::get_statement(size_t trh, size_t sth)
{
    return get_transaction(trh)->get_statement(sth);
}

std::unique_ptr<Blob> &Database::get_blob(size_t trh, size_t blh)
{
    return get_transaction(trh)->get_blob(blh);
}

} // namespace
