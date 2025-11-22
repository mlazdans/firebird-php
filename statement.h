#ifndef STATEMENT_H
#define STATEMENT_H

#include <ibase.h>
#include "php.h"

#define FBP_FETCH_BLOB_TEXT (1<<0)
#define FBP_FETCH_UNIXTIME  (1<<1)
#define FBP_FETCH_DATE_OBJ  (1<<2)
#define FBP_FETCH_BLOB_OBJ  (1<<3)
#define FBP_FETCH_INDEXED   (1<<4)
#define FBP_FETCH_HASHED    (1<<5)

void FireBird_Statement___construct(zval *self, zval *transaction);
int FireBird_Statement_prepare(zval *self, zend_string *sql);
int FireBird_Statement_execute(zval *self, zval *bind_args, uint32_t num_bind_args);
void FireBird_Statement_fetch(zval *self, int flags, zval *return_value);

void register_FireBird_Statement_object_handlers();

#endif /* STATEMENT_H */
