#pragma once

#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "./base.hpp"
#include "./statement.hpp"
#include "../firebird_php.hpp"

#define TRANS_ID_SIZE 7

extern "C" {

// Same values as MON$TRANSACTIONS.MON$ISOLATION_MODE
enum fbp_isolation_mode {
    FBP_ISOLATION_CONSISTENCY = 0,          // consistency (snapshot table stability)
    FBP_ISOLATION_CONCURRENCY = 1,          // concurrency (snapshot)
    FBP_ISOLATION_RC_REC_VERSION = 2,       // read committed record version
    FBP_ISOLATION_RC_NO_REC_VERSION = 3,    // read committed no record version
    FBP_ISOLATION_RC_READ_CONSISTENCY = 4   // read committed read consistency
};

typedef struct firebird_tbuilder {
    char read_only, ignore_limbo, auto_commit, no_auto_undo;

    fbp_isolation_mode isolation_mode;
    ISC_SHORT lock_timeout;
    ISC_UINT64 snapshot_at_number;

    zend_object std;
} firebird_tbuilder;

typedef struct firebird_trans_info {
    ISC_INT64 id;
} firebird_trans_info;

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
private:
    Database &dba;
    ITransaction *tra = nullptr;
    firebird_trans_info info = {0};
    bool is_prepared = false;
public:
    Transaction(Database &dba);
    ~Transaction() noexcept;
    ITransaction *get();
    Database &get_database() { return dba; }
    // Started and not yet committed, rolled back or prepared for two-phase commit
    bool is_active() const { return tra && !is_prepared; }
    void set(ITransaction *tra);
    void start(const firebird_tbuilder *builder);
    void query_info();
    IXpbBuilder *build_tpb(const firebird_tbuilder *builder);
    void commit();
    void commit_ret();
    void rollback();
    void rollback_ret();
    const firebird_trans_info *get_info();

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
        IMessageMetadata* output_metadata, unsigned flags = 0);

    void finalize(int mode);
    void prepare();
};

} // namespace

extern "C"
{
#include "zend_exceptions.h"

typedef struct firebird_trans {
    size_t dbh;
    size_t trh;

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
void register_FireBird_Transaction_object_handlers();
void register_FireBird_Transaction_Builder_object_handlers();

}
