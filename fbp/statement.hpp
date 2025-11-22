#pragma once

#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/transaction.hpp"
#include "fbp/statement.hpp"

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
public:
    unsigned int statement_type = 0;
    unsigned int in_vars_count = 0, out_vars_count = 0;
    unsigned int in_buffer_size = 0, out_buffer_size = 0;
public:
    Statement(Transaction *tra);
    ~Statement() noexcept;
    void prepare(unsigned int len_sql, const char *sql);
    IStatement* get_statement();
    void bind(zval *b_vars, unsigned int num_bind_args);
    void open_cursor();
    int fetch_next();
    void output_buffer_to_array(zval *hash, int flags);
    int var_zval(zval *val, unsigned int index, int flags);
    void execute();
    // void execute_on_att(unsigned len_sql, const char *sql);
};

}
