#pragma once

#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/transaction.hpp"

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
    std::vector<std::unique_ptr<Statement>> st_list;
    std::vector<std::unique_ptr<Blob>> bl_list;
public:
    std::unique_ptr<Transaction> &get_transaction(size_t trh);
    std::unique_ptr<Statement> &get_statement(size_t sth);
    std::unique_ptr<Blob> &get_blob(size_t blh);
    Database();
    ~Database();
    IAttachment *get();
    void connect(zval *args);
    void create(zval *args);
    void disconnect();
    void drop();
    void execute_create(unsigned int len_sql, const char *sql);

    size_t transaction_init();
    size_t statement_init(size_t trh);
    size_t blob_init(size_t trh);
    void transaction_free(size_t trh);
    void statement_free(size_t sth);
    void blob_free(size_t blh);
};

} // namespace

extern "C" {

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