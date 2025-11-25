#pragma once

#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "fbp/base.hpp"

using namespace Firebird;

namespace FBP {

class Transaction;
class Statement;
class Blob;

class Database: Base {
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = default;
    Database& operator=(Database&&) = default;
private:
    IAttachment *att = nullptr;
    std::vector<std::unique_ptr<Transaction>> tr_list;
public:
    Database();
    ~Database();
    void connect(zval *args);
    void create(zval *args);
    void disconnect();
    void drop();
    ITransaction *execute(ITransaction *tra, unsigned len_sql, const char *sql,
        IMessageMetadata *im, unsigned char *in_buffer,
        IMessageMetadata *om, unsigned char *out_buffer);
    IBlob *open_blob(ITransaction *transaction, ISC_QUAD *blob_id);
    IBlob *create_blob(ITransaction *transaction, ISC_QUAD *blob_id);
    IStatement *prepare(ITransaction *transaction, unsigned int len_sql, const char *sql);
    ITransaction *start_transaction(unsigned int tpb_len, const unsigned char* tpb);
    size_t new_transaction();
    std::unique_ptr<Transaction> &get_transaction(size_t trh);
    std::unique_ptr<Statement> &get_statement(size_t trh, size_t sth);
    std::unique_ptr<Blob> &get_blob(size_t trh, size_t blh);
};

} // namespace

extern "C" {

extern zend_class_entry *FireBird_Database_ce;
extern zend_class_entry *FireBird_Db_Info_ce;
extern zend_class_entry *FireBird_Connect_Args_ce;
extern zend_class_entry *FireBird_Create_Args_ce;

extern firebird_xpb_zmap fbp_database_create_zmap;
extern firebird_xpb_zmap fbp_database_connect_zmap;
extern firebird_xpb_zmap fbp_database_info_zmap;

typedef struct firebird_db {
    size_t dbh;

    zend_object std;
} firebird_db;

fbp_declare_object_accessor(firebird_db);

void register_FireBird_Database_object_handlers();
void FireBird_Database_reconnect_transaction(zval *Db, zval *return_value, zend_long id);

int fbp_database_build_dpb(zend_class_entry *ce, zval *Args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written);
int fbp_database_get_info(firebird_db *db, zval *Db_Info,
    size_t info_req_size, char *info_req, size_t info_resp_size, char *info_resp, size_t max_limbo_count);

}