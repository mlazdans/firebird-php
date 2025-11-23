#ifndef BLOB_H
#define BLOB_H

#include <ibase.h>
#include "php.h"

#include "transaction.h"

typedef struct firebird_blob_info {
    ISC_QUAD id;
    ISC_LONG max_segment;
    ISC_LONG num_segments;
    ISC_LONG total_length;
    ISC_LONG type; // 0 - segmented, 1 - streamed
    ISC_LONG position;
    unsigned short is_writable;
} firebird_blob_info;

typedef struct firebird_blob {
    void *blobptr;
    firebird_trans *tr;
    firebird_blob_info *info;

    zend_object std;
} firebird_blob;

fbp_declare_object_accessor(firebird_blob);

typedef struct firebird_blob_id {
    ISC_QUAD bl_id;

    zend_object std;
} firebird_blob_id;

#define FBP_BLOB_TYPE_SEGMENTED isc_bpb_type_segmented
#define FBP_BLOB_TYPE_STREAMED  isc_bpb_type_stream

#define FBP_BLOB_SEEK_START   0
#define FBP_BLOB_SEEK_CURRENT 1
#define FBP_BLOB_SEEK_END     2

fbp_declare_object_accessor(firebird_blob_id);

extern zend_class_entry *FireBird_Blob_ce;
extern zend_class_entry *FireBird_Blob_Id_ce;

void register_FireBird_Blob_object_handlers();
void register_FireBird_Blob_Id_object_handlers();

void FireBird_Blob___construct(zval *self, zval *Transaction);
int FireBird_Blob_create(zval *self);
int FireBird_Blob_open(zval *self, zval *Blob_Id);
int FireBird_Blob_put(zval *self, const char *buf, unsigned int buf_size);
int FireBird_Blob_close(zval *self);
int FireBird_Blob_cancel(zval *self);
int FireBird_Blob_get(zval *self, zval *return_value, size_t max_len);

void FireBird_Blob_Id___construct(zval *self, ISC_QUAD bl_id);

int fbp_blob_id_to_string(ISC_QUAD const qd, size_t buf_len, char *buf);
int fbp_blob_id_to_quad(size_t id_len, char const *id, ISC_QUAD *qd);

#endif /* BLOB_H */