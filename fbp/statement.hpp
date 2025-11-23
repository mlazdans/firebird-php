#pragma once

#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/transaction.hpp"

using namespace Firebird;

namespace FBP {

class Statement: Base
{
private:
    Transaction *tra;
    IStatement *statement = nullptr;
    IResultSet *cursor = nullptr;
    IMessageMetadata *input_metadata = nullptr, *output_metadata = nullptr;
    unsigned char *in_buffer = nullptr, *out_buffer = nullptr;
    HashTable *ht_aliases = nullptr, *ht_ind = nullptr;

    void insert_alias(const char *alias);
    void alloc_ht_aliases();
    void alloc_ht_ind();
public:
    unsigned int statement_type = 0;
    unsigned int in_vars_count = 0, out_vars_count = 0;
    unsigned int in_buffer_size = 0, out_buffer_size = 0;

    Statement(Transaction *tra);
    ~Statement() noexcept;
    void prepare(unsigned int len_sql, const char *sql);
    IStatement* get_statement();
    void bind(zval *b_vars, unsigned int num_bind_args);
    void open_cursor();
    int close_cursor();
    int fetch_next();
    HashTable *output_buffer_to_array(int flags);
    int var_zval(zval *val, unsigned int index, int flags);
    void execute();
};

}
