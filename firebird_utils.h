#ifndef FIREBIRD_UTILS_H
#define FIREBIRD_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ibase.h>
#include "database.h"
#include "database.h"
#include "transaction.h"
#include "fbp_statement.h"
#include "fbp_blob.h"

#define STRNUM_PARSE_OK       0
#define STRNUM_PARSE_ERROR    1
#define STRNUM_PARSE_OVERFLOW 2

unsigned fbu_get_client_version(void);
// int fbu_start_transaction(ISC_STATUS* status, const firebird_db *db, const firebird_tbuilder *builder, firebird_trans *tr);
int fbu_execute_database(ISC_STATUS* status, const firebird_db *db, size_t len_sql, const char *sql, firebird_trans *tr);
int fbu_blob_open(ISC_STATUS* status, firebird_trans *tr, ISC_QUAD id, firebird_blob *blob);
int fbu_blob_close(ISC_STATUS* status, firebird_blob *blob);
int fbu_blob_get_segment(ISC_STATUS* status, firebird_blob *blob, zend_string *buf, unsigned* len);

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
int fbu_database_execute(firebird_db *db, unsigned len_sql, const char *sql);

int fbu_transaction_init(firebird_db *db, firebird_trans *tr);
int fbu_transaction_free(firebird_trans *tr);
int fbu_transaction_start(firebird_trans *tr, const firebird_tbuilder *builder);
int fbu_transaction_execute(firebird_trans *tr, size_t len_sql, const char *sql);
int fbu_transaction_finalize(firebird_trans *tr, int mode);

int fbu_statement_prepare(firebird_trans *tr, unsigned len_sql, const char *sql, firebird_stmt *stmt);
void fbu_statement_free(firebird_stmt *stmt);
int fbu_statement_bind(firebird_stmt *stmt, zval *b_vars, unsigned int num_bind_args);
int fbu_statement_open_cursor(firebird_stmt *stmt);
int fbu_statement_fetch_next(firebird_stmt *stmt);
int fbu_statement_output_buffer_to_array(firebird_stmt *stmt, zval *hash, int flags);
int fbu_statement_execute(firebird_stmt *stmt);
int fbu_statement_execute_on_att(firebird_stmt *stmt);

#ifdef __cplusplus
}
#endif

#endif  // FIREBIRD_UTILS_H
