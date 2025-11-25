#pragma once

#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/statement.hpp"

extern "C" {

typedef struct firebird_tbuilder {
    char read_only, ignore_limbo, auto_commit, no_auto_undo;

    /**
     * Isolation mode (level):
     * 0 - consistency (snapshot table stability)
     * 1 - concurrency (snapshot)
     * 2 - read committed record version
     * 3 - read committed no record version
     * 4 - read committed read consistency
     */
    char isolation_mode;
    ISC_SHORT lock_timeout;
    ISC_UINT64 snapshot_at_number;

    zend_object std;
} firebird_tbuilder;

}

using namespace Firebird;

#define FBP_TR_ROLLBACK 1
#define FBP_TR_COMMIT   2
#define FBP_TR_RETAIN   4

namespace FBP {

class Database;
class Statement;
class Blob;

class Transaction: Base
{
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) = default;
    Transaction& operator=(Transaction&&) = default;
private:
    Database &dba;
    ITransaction *tra = nullptr;
    std::vector<std::unique_ptr<Statement>> st_list;
    std::vector<std::unique_ptr<Blob>> bl_list;
public:
    Transaction(Database &dba);
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
    size_t new_statement();
    size_t new_blob();
    std::unique_ptr<Statement> &get_statement(size_t sth);
    std::unique_ptr<Blob> &get_blob(size_t blh);
};

} // namespace

extern "C"
{
#include "zend_exceptions.h"

typedef struct firebird_trans {
    size_t dbh;
    size_t trh;
    ISC_INT64 id;

    zend_object std;
} firebird_trans;

typedef struct firebird_teb {
    isc_db_handle *db_handle;
    int tpb_len;
    char *tpb;
} firebird_teb;

fbp_declare_object_accessor(firebird_tbuilder);
fbp_declare_object_accessor(firebird_trans);

int FireBird_Transaction___construct(zval *self, zval *database);
int FireBird_Transaction_start(zval *self, zval *builder);
// int FireBird_Transaction_prepare(zval *Tr, zval *return_Stmt, const ISC_SCHAR* sql);
void register_FireBird_Transaction_object_handlers();
void register_FireBird_TBuilder_object_handlers();

}
