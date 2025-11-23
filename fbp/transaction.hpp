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
    void start(const firebird_tbuilder *builder);
    ISC_INT64 query_transaction_id();
    static void fbu_transaction_build_tpb(IXpbBuilder *tpb, const firebird_tbuilder *builder);
    int commit();
    int commit_ret();
    int rollback();
    int rollback_ret();
    zend_string *get_blob_contents(ISC_QUAD *blob_id);
    ISC_QUAD create_blob(zend_string *data);
    ITransaction *execute(unsigned len_sql, const char *sql);
    IBlob *open_blob(ISC_QUAD *blob_id);
    IBlob *create_blob(ISC_QUAD *blob_id);
    void execute_statement(IStatement *statement,
        IMessageMetadata* input_metadata, void* in_buffer,
        IMessageMetadata* output_metadata, void* out_buffer);
    IResultSet *open_cursor(IStatement *statement,
        IMessageMetadata* input_metadata, void* in_buffer,
        IMessageMetadata* output_metadata);
    IStatement* prepare(unsigned int len_sql, const char *sql);
};

} // namespace
