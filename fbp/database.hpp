#pragma once

#include <vector>
#include <memory>
#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/transaction.hpp"

using namespace Firebird;

namespace FBP {

    class Transaction;

class Database: Base {
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;
private:
    zval *args;
    IAttachment *att = nullptr;
    ITransaction *internal_tra = nullptr;
    // std::vector<std::unique_ptr<Transaction>> trans_list;
public:
    Database(zval *args);
    ~Database();
    IAttachment *get_att();
    // Transaction *start_transaction(const firebird_tbuilder *builder);
    int connect();
    int create();
    int disconnect();
    int drop();
    // void execute(unsigned len_sql, const char *sql,
    //     IMessageMetadata *im, unsigned char *in_buffer,
    //     IMessageMetadata *om, unsigned char *out_buffer);
    // void register_transaction(std::unique_ptr<Transaction>);
    // Transaction *new_transaction();
};

} // namespace