#include <vector>
#include <memory>
#include <firebird/Interface.h>
#include <stdint.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "database.h"

#include "firebird_utils.h"
// #include "zend_exceptions.h"

using namespace Firebird;

namespace FBP {

Database::Database(zval *args) : args(args)
{
}

IAttachment* Database::get_att() {
    FBDEBUG("Database::get_att(): %p", att);
    return att;
}

// int prepare(firebird_trans *tr, unsigned len_sql, const unsigned char *sql, firebird_stmt *stmt)
// {
//     if (!tr || !tr->trptr) return false;
//     auto tra = static_cast<Transaction *>(tr->trptr);

//     auto s = att->prepare(&st, tra, len_sql, sql, SQL_DIALECT_CURRENT,
//         IStatement::PREPARE_PREFETCH_METADATA
//     );

// }

// Transaction* Database::start_transaction(const firebird_tbuilder *builder)
// {
//     auto tr = new Transaction(*this);
//     tr->start(builder);
//     // trans_list.emplace_back(tr);
//     return tr;
// }

int Database::connect()
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

    return SUCCESS;
}

int Database::create()
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

    return SUCCESS;
}

int Database::disconnect()
{
    if (att) {
        att->detach(&st);
        att->release();
        att = nullptr;

        return SUCCESS;
    } else {
        return 1;
    }
}

int Database::drop()
{
    att->dropDatabase(&st);
    att->release();
    att = nullptr;

    return SUCCESS;
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

    if (err) fbu_handle_exception2();
}

// void Database::execute(unsigned len_sql, const char *sql,
//         IMessageMetadata* im, unsigned char *in_buffer,
//         IMessageMetadata* om, unsigned char *out_buffer)
// {
//     internal_tra = get_att()->execute(&st, internal_tra, len_sql, sql, SQL_DIALECT_CURRENT, im, in_buffer, om, out_buffer);
// }

// void Database::register_transaction(Transaction &tra)
// {
//     trans_list.emplace_back(tra);
// }

// Transaction* Database::new_transaction()
// {
//     return new Transaction(this);
// }

} // namespace
