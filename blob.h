#ifndef BLOB_H
#define BLOB_H

#include <ibase.h>
#include "php.h"

extern zend_class_entry *FireBird_Blob_ce;
extern zend_class_entry *FireBird_Blob_Id_ce;

void register_FireBird_Blob_Id_ce();
void register_FireBird_Blob_ce();

void FireBird_Blob___construct(zval *Blob, zval *Transaction);
void FireBird_Blob_Id___construct(zval *Blob_Id, ISC_QUAD bl_id);
int FireBird_Blob_close(zval *Blob);
int FireBird_Blob_cancel(zval *Blob);
int FireBird_Blob_get(zval *Blob, zval *return_value, size_t max_len);
int FireBird_Blob_put(zval *Blob, const char *buf, size_t buf_size);
int FireBird_Blob_open(zval *Blob, zval *Blob_Id);
int FireBird_Blob_create(zval *Blob);

#endif /* BLOB_H */