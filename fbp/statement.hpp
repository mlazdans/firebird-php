#pragma once

#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/transaction.hpp"

#include "firebird_php.hpp"

#define FBP_FETCH_BLOB_TEXT (1<<0)
#define FBP_FETCH_UNIXTIME  (1<<1)
#define FBP_FETCH_DATE_OBJ  (1<<2)
#define FBP_FETCH_BLOB_OBJ  (1<<3)
#define FBP_FETCH_INDEXED   (1<<4)
#define FBP_FETCH_HASHED    (1<<5)

extern "C" {
typedef struct firebird_field {
    const char *alias;
    unsigned int type;
    unsigned int len;
    int scale;
    unsigned int offset;
    unsigned int null_offset;
} firebird_field;

typedef struct firebird_stmt_info {
    unsigned char statement_type;
    const char *sql;
    unsigned int sql_len;

    const char *name;
    ISC_ULONG insert_count, update_count, delete_count, select_count, affected_count;

    unsigned int in_vars_count, out_vars_count;
} firebird_stmt_info;

typedef struct firebird_stmt {
    size_t dbh;
    size_t trh;
    size_t sth;
    firebird_stmt_info *info;

    unsigned char did_fake_fetch, is_cursor_open, is_exhausted;

    zend_object std;
} firebird_stmt;

fbp_declare_object_accessor(firebird_stmt);

typedef struct firebird_vary {
    unsigned short vary_length;
    unsigned char vary_string[1];
} firebird_vary;

int FireBird_Statement___construct(zval *self, zval *transaction);
int FireBird_Statement_prepare(zval *self, zend_string *sql);
int FireBird_Statement_execute(zval *self, zval *bind_args, uint32_t num_bind_args);
void FireBird_Statement_fetch(zval *self, int flags, zval *return_value);

void register_FireBird_Statement_object_handlers();

}

using namespace Firebird;

namespace FBP {

class Transaction;

class Statement: Base
{
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&&) = default;
    Statement& operator=(Statement&&) = default;
private:
    Transaction &tra;
    IStatement *statement = nullptr;
    IResultSet *cursor = nullptr;
    IMessageMetadata *input_metadata = nullptr, *output_metadata = nullptr;
    unsigned char *in_buffer = nullptr, *out_buffer = nullptr;
    HashTable *ht_aliases = nullptr, *ht_ind = nullptr;
    firebird_stmt_info info = {0};
    std::vector<firebird_field> in_fields;
    std::vector<firebird_field> out_fields;

    void insert_alias(const char *alias);
    void alloc_ht_aliases();
    void alloc_ht_ind();
    void query_statistics();
public:
    unsigned int in_buffer_size = 0, out_buffer_size = 0;

    Statement(Transaction &tra);
    ~Statement() noexcept;
    void prepare(unsigned int len_sql, const char *sql);
    void bind(zval *b_vars, unsigned int num_bind_args);
    void open_cursor();
    int close_cursor();
    int fetch_next();
    HashTable *output_buffer_to_array(int flags);
    int var_zval(zval *val, unsigned int index, int flags);
    void execute();
    firebird_stmt_info *get_info();
};

}
