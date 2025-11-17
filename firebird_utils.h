/*
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | https://www.php.net/license/3_01.txt                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Simonov Denis <sim-mail@list.ru>                             |
  +----------------------------------------------------------------------+
*/

#ifndef FIREBIRD_UTILS_H
#define FIREBIRD_UTILS_H

#include <ibase.h>
#include "database.h"
#include "fbp_database.h"
#include "fbp_transaction.h"
#include "fbp_statement.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned fbu_get_client_version(void);
ISC_TIME fbu_encode_time(unsigned hours, unsigned minutes, unsigned seconds, unsigned fractions);
ISC_DATE fbu_encode_date(unsigned year, unsigned month, unsigned day);
void fbu_decode_time_tz(const ISC_TIME_TZ* timeTz, unsigned* hours, unsigned* minutes,
    unsigned* seconds, unsigned* fractions, unsigned timeZoneBufferLength, char* timeZoneBuffer);
void fbu_decode_timestamp_tz(const ISC_TIMESTAMP_TZ* timestampTz,
    unsigned* year, unsigned* month, unsigned* day,
    unsigned* hours, unsigned* minutes, unsigned* seconds, unsigned* fractions,
    unsigned timeZoneBufferLength, char* timeZoneBuffer);
int fbu_attach_database(ISC_STATUS* status, firebird_db *db, zval *Connect_Args, zend_class_entry *ce);
int fbu_detach_database(ISC_STATUS* status, firebird_db *db);
int fbu_start_transaction(ISC_STATUS* status, firebird_trans *tr);
int fbu_finalize_transaction(ISC_STATUS* status, firebird_trans *tr, int mode);
int fbu_prepare_statement(ISC_STATUS* status, firebird_stmt *stmt, const char *sql);
int fbu_free_statement(ISC_STATUS* status, firebird_stmt *stmt);
int fbu_execute_statement(ISC_STATUS* status, firebird_stmt *stmt);
int fbu_open_cursor(ISC_STATUS* status, firebird_stmt *stmt);
int fbu_fetch(ISC_STATUS* status, firebird_stmt *stmt, int flags, zval *return_value);
int fbu_statement_bind(ISC_STATUS* status, firebird_stmt *stmt, zval *b_vars, size_t num_bind_args);
#ifdef __cplusplus
}
#endif

#endif  // FIREBIRD_UTILS_H
