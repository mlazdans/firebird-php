#ifndef STATEMENT_H
#define STATEMENT_H

#include <ibase.h>
#include "php.h"

typedef enum {
    FBP_FETCH_BLOBS      = 1,
    FBP_FETCH_UNIXTIME   = 2,
} firebird_fetch_flag;

extern zend_class_entry *FireBird_Statement_ce;
extern void register_FireBird_Statement_ce();

void FireBird_Statement___construct(zval *Stmt, zval *Transaction);
int FireBird_Statement_execute(zval *Stmt, zval *bind_args, uint32_t num_bind_args);
int FireBird_Statement_prepare(zval *Stmt, const ISC_SCHAR* sql);
void FireBird_Statement_fetch(zval *Stmt, zval *return_value, firebird_fetch_flag flags, int return_type);

#endif /* STATEMENT_H */
