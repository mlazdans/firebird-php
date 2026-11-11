#pragma once

#include <map>
#include <string>
#include <firebird/Interface.h>

#include "./base.hpp"
#include "./database.hpp"
#include "./transaction.hpp"
#include "../firebird_php.hpp"

#define FBP_FETCH_BLOB_TEXT (1<<0)
#define FBP_FETCH_UNIXTIME  (1<<1)
#define FBP_FETCH_INDEXED   (1<<4)
#define FBP_FETCH_HASHED    (1<<5)

extern "C" {
typedef struct firebird_var {
    const char *field;
    const char *alias;
    const char *relation;
    unsigned int type;
    int subtype;
    unsigned int length;
    int scale;
    int is_nullable;
    unsigned int offset;
    unsigned int null_offset;
} firebird_var;

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
private:
    Transaction &tra;
    IStatement *stmt = nullptr;
    IResultSet *cursor = nullptr;
    IMessageMetadata *input_metadata = nullptr, *output_metadata = nullptr;
    unsigned char *in_buffer = nullptr, *out_buffer = nullptr;
    HashTable *ht_aliases = nullptr, *ht_ind = nullptr;
    firebird_stmt_info info = {0};
    std::vector<firebird_var> in_vars;
    std::vector<firebird_var> out_vars;

    void insert_alias(const char *alias);
    void alloc_ht_aliases();
    void alloc_ht_ind();
    bool is_executed = false, cursor_exhausted = false;

    void bind_datetime(zval *b_var, unsigned argn, const firebird_var &var,
        unsigned char *sqldata, const char *type_named);

    // One DateTime per zone ("" for date.timezone), updated for each date/time
    // cell and cloned for the value returned
    std::map<std::string, zend_object*> date_scratch;
    zend_object *date_scratch_for(const char *zone);
    void make_date_obj(zval *val, unsigned type, unsigned y, unsigned mo, unsigned d,
        unsigned h, unsigned mi, unsigned sec, unsigned frac, const char *zone, int64_t epoch);

    void query_statistics();
    void free_cursor();
    bool is_select() const;
public:
    unsigned int in_buffer_size = 0, out_buffer_size = 0;

    Statement(Transaction &tra);
    ~Statement() noexcept;
    void prepare(unsigned int len_sql, const char *sql);
    void bind(zval *b_vars, unsigned int num_bind_args);
    void open_cursor();
    void close_cursor();
    int fetch_next();
    void output_buffer_to_array(int flags, HashTable **hash);
    int var_zval(zval *val, unsigned int index, int flags);
    void execute();
    const firebird_stmt_info *get_info();
    IStatement *get();
    IResultSet *get_cursor();
    void get_var_info(int in, unsigned int index, zval *var_info);
};

}
