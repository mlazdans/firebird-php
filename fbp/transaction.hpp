#pragma once

#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "firebird_utils.h"
#include "zend_exceptions.h"

using namespace Firebird;

namespace FBP {

class Database;

class Transaction: Base
{
private:
    Database *dba;
    ITransaction *tra = nullptr;
public:
    Transaction(Database *dba);
    ~Transaction() noexcept;
    ITransaction *get_tra();
    IAttachment *get_dba();
    void start(const firebird_tbuilder *builder);
    ISC_INT64 query_transaction_id();
    static void fbu_transaction_build_tpb(IXpbBuilder *tpb, const firebird_tbuilder *builder);
    void commit();
    void commit_ret();
    void rollback();
    void rollback_ret();
    zend_string* fetch_blob_data(ISC_QUAD id);
    int blob_set_info(IBlob *blo, firebird_blob *blob);
    ITransaction *execute(unsigned len_sql, const char *sql);
};

} // namespace
