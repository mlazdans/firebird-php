#pragma once

#include "fbp/base.hpp"
#include "fbp/blob.hpp"
#include "fbp/statement.hpp"

#define STRNUM_PARSE_OK       0
#define STRNUM_PARSE_ERROR    1
#define STRNUM_PARSE_OVERFLOW 2


extern "C" {

void fbu_handle_exception(const char *file, size_t line);
void fbu_xpb_insert_object(Firebird::IXpbBuilder* xpb, zval *obj, zend_class_entry *ce,
    const firebird_xpb_zmap *xpb_zmap);

#include <ibase.h>

unsigned fbu_get_client_version(void);

const char* fbu_get_sql_type_name(unsigned type);
int fbu_string_to_numeric(const char *s, size_t slen, int scale, uint64_t max,
    int *sign, int *exp, uint64_t *res);
void fbu_init_date_object(const char *tzbuff, zval *o);

int fbu_is_valid_dbh(size_t dbh);
int fbu_is_valid_trh(size_t dbh, size_t trh);
int fbu_is_valid_sth(size_t dbh, size_t sth);

int fbu_database_init(size_t *dbh);
int fbu_database_free(size_t dbh);
int fbu_database_drop(size_t dbh);
int fbu_database_disconnect(size_t dbh);
int fbu_database_connect(size_t dbh, zval *args);
int fbu_database_create(size_t dbh, zval *args);

int fbu_transaction_init(size_t dbh, size_t *trh);
int fbu_transaction_get_info(size_t dbh, size_t trh, const firebird_trans_info **info);
int fbu_transaction_start(size_t dbh, size_t trh, const firebird_tbuilder *builder);
int fbu_transaction_free(size_t dbh, size_t trh);
int fbu_transaction_finalize(size_t dbh, size_t trh, int mode);
int fbu_transaction_execute(size_t dbh, size_t trh, size_t len_sql, const char *sql);

int fbu_statement_init(size_t dbh, size_t trh, size_t *sth);
int fbu_statement_get_info(size_t dbh, size_t sth, const firebird_stmt_info **info);
int fbu_statement_free(size_t dbh, size_t sth);
int fbu_statement_prepare(size_t dbh, size_t sth, unsigned len_sql, const char *sql);
int fbu_statement_bind(size_t dbh, size_t sth, zval *b_vars, unsigned int num_bind_args);
int fbu_statement_open_cursor(size_t dbh, size_t sth);
int fbu_statement_fetch_next(size_t dbh, size_t sth, int *istatus);
int fbu_statement_output_buffer_to_array(size_t dbh, size_t sth, int flags, HashTable **ht);
int fbu_statement_execute(size_t dbh, size_t sth);
int fbu_statement_close_cursor(size_t dbh, size_t sth);

int fbu_blob_init(size_t dbh, size_t trh, size_t *blh);
int fbu_blob_get_info(size_t dbh, size_t blh, const firebird_blob_info **info);
int fbu_blob_free(size_t dbh, size_t blh);
int fbu_blob_open(size_t dbh, size_t blh, ISC_QUAD *bl_id);
int fbu_blob_create(size_t dbh, size_t blh);
int fbu_blob_close(size_t dbh, size_t blh);
int fbu_blob_cancel(size_t dbh, size_t blh);
int fbu_blob_get(size_t dbh, size_t blh, int max_len, zend_string **buf);
int fbu_blob_put(size_t dbh, size_t blh, unsigned int buf_size, const char *buf);
int fbu_blob_seek(size_t dbh, size_t blh, int mode, int offset, int *new_offset);

}
