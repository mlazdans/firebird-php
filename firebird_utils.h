#ifndef FIREBIRD_UTILS_H
#define FIREBIRD_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ibase.h>
#include "database.h"
#include "database.h"
#include "transaction.h"
#include "statement.h"
#include "blob.h"

#define STRNUM_PARSE_OK       0
#define STRNUM_PARSE_ERROR    1
#define STRNUM_PARSE_OVERFLOW 2

unsigned fbu_get_client_version(void);

const char* fbu_get_sql_type_name(unsigned type);
int fbu_string_to_numeric(const char *s, const size_t slen, int scale, uint64_t max,
    int *sign, int *exp, uint64_t *res);
void fbu_init_date_object(const char *tzbuff, zval *o);

int fbu_database_init(zval *args, firebird_db *db);
int fbu_database_free(firebird_db *db);
int fbu_database_connect(firebird_db *db);
int fbu_database_create(firebird_db *db);
int fbu_database_disconnect(firebird_db *db);
int fbu_database_drop(firebird_db *db);

int fbu_transaction_init(firebird_db *db, firebird_trans *tr);
int fbu_transaction_free(firebird_trans *tr);
int fbu_transaction_start(firebird_trans *tr, const firebird_tbuilder *builder);
int fbu_transaction_execute(firebird_trans *tr, size_t len_sql, const char *sql);
int fbu_transaction_finalize(firebird_trans *tr, int mode);

int fbu_statement_init(firebird_trans *tr, firebird_stmt *stmt);
int fbu_statement_prepare(firebird_stmt *stmt, unsigned len_sql, const char *sql);
void fbu_statement_free(firebird_stmt *stmt);
int fbu_statement_bind(firebird_stmt *stmt, zval *b_vars, unsigned int num_bind_args);
int fbu_statement_open_cursor(firebird_stmt *stmt);
int fbu_statement_fetch_next(firebird_stmt *stmt);
HashTable *fbu_statement_output_buffer_to_array(firebird_stmt *stmt, int flags);
int fbu_statement_execute(firebird_stmt *stmt);
int fbu_statement_close_cursor(firebird_stmt *stmt);

int fbu_blob_init(firebird_trans *tr, firebird_blob *blob);
int fbu_blob_free(firebird_blob *blob);
int fbu_blob_open(firebird_blob *blob, firebird_blob_id *blob_id);
int fbu_blob_create(firebird_blob *blob);
int fbu_blob_close(firebird_blob *blob);
int fbu_blob_cancel(firebird_blob *blob);
int fbu_blob_put(firebird_blob *blob, unsigned int buf_size, const char *buf);
zend_string *fbu_blob_get(firebird_blob *blob, unsigned int max_len);
int fbu_blob_seek(firebird_blob *blob, int mode, int offset, int *new_offset);

#ifdef __cplusplus
}
#endif

#endif  // FIREBIRD_UTILS_H
