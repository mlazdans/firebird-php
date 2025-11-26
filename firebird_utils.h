#pragma once

#include "fbp/base.hpp"
#include "fbp/blob.hpp"
#include "fbp/statement.hpp"

#define STRNUM_PARSE_OK       0
#define STRNUM_PARSE_ERROR    1
#define STRNUM_PARSE_OVERFLOW 2

void fbu_handle_exception(const char *file, size_t line);

extern "C" {

void fbu_xpb_insert_object(Firebird::IXpbBuilder* xpb, zval *obj, zend_class_entry *ce,
    const firebird_xpb_zmap *xpb_zmap);

#include <ibase.h>

unsigned fbu_get_client_version(void);

const char* fbu_get_sql_type_name(unsigned type);
int fbu_string_to_numeric(const char *s, const size_t slen, int scale, uint64_t max,
    int *sign, int *exp, uint64_t *res);
void fbu_init_date_object(const char *tzbuff, zval *o);

int fbu_is_valid_dbh(firebird_db *db);
int fbu_is_valid_trh(firebird_trans *tr);
int fbu_is_valid_sth(firebird_stmt *stmt);

int fbu_database_init(firebird_db *db);
int fbu_database_free(firebird_db *db);
int fbu_database_drop(firebird_db *db);
int fbu_database_disconnect(firebird_db *db);
int fbu_database_connect(const firebird_db *db, zval *args);
int fbu_database_create(const firebird_db *db, zval *args);

int fbu_transaction_init(const firebird_db *db, firebird_trans *tr);
int fbu_transaction_start(firebird_trans *tr, const firebird_tbuilder *builder);
int fbu_transaction_free(firebird_trans *tr);
int fbu_transaction_finalize(const firebird_trans *tr, int mode);
int fbu_transaction_execute(const firebird_trans *tr, size_t len_sql, const char *sql);

int fbu_statement_init(const firebird_trans *tr, firebird_stmt *stmt);
int fbu_statement_free(firebird_stmt *stmt);
int fbu_statement_prepare(const firebird_stmt *stmt, unsigned len_sql, const char *sql);
int fbu_statement_bind(firebird_stmt *stmt, zval *b_vars, unsigned int num_bind_args);
int fbu_statement_open_cursor(firebird_stmt *stmt);
int fbu_statement_fetch_next(firebird_stmt *stmt);
HashTable *fbu_statement_output_buffer_to_array(const firebird_stmt *stmt, int flags);
int fbu_statement_execute(const firebird_stmt *stmt);
int fbu_statement_close_cursor(const firebird_stmt *stmt);

int fbu_blob_init(const firebird_trans *tr, firebird_blob *blob);
int fbu_blob_free(firebird_blob *blob);
int fbu_blob_open(const firebird_blob *blob, firebird_blob_id *blob_id);
int fbu_blob_create(const firebird_blob *blob);
int fbu_blob_close(const firebird_blob *blob);
int fbu_blob_cancel(const firebird_blob *blob);
int fbu_blob_put(const firebird_blob *blob, unsigned int buf_size, const char *buf);
zend_string *fbu_blob_get(const firebird_blob *blob, unsigned int max_len);
int fbu_blob_seek(const firebird_blob *blob, int mode, int offset, int *new_offset);

}
