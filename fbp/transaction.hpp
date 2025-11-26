#pragma once

#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/statement.hpp"
#include "firebird_php.hpp"

// For limbo buffers. Need roughly 7 bytes per transaction id
#define TRANS_ID_SIZE 8
#define TRANS_MAX_STACK_COUNT 128

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
public:
    Transaction(Database &dba);
    ~Transaction() noexcept;
    ITransaction *get();
    void start(const firebird_tbuilder *builder);
    ISC_INT64 query_transaction_id();
    IXpbBuilder *build_tpb(const firebird_tbuilder *builder);
    void commit();
    void commit_ret();
    void rollback();
    void rollback_ret();

    ITransaction *execute(unsigned len_sql, const char *sql);
    IBlob *open_blob(ISC_QUAD *blob_id);
    IBlob *create_blob(ISC_QUAD *blob_id);
    ISC_QUAD create_blob_from_string(zend_string *data);
    zend_string *get_blob_contents(ISC_QUAD *blob_id);

    void execute_statement(IStatement *statement,
        IMessageMetadata* input_metadata, void* in_buffer,
        IMessageMetadata* output_metadata, void* out_buffer);
    IResultSet *open_cursor(IStatement *statement,
        IMessageMetadata* input_metadata, void* in_buffer,
        IMessageMetadata* output_metadata);
    IStatement* prepare(unsigned int len_sql, const char *sql);

    void finalize(int mode);
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
