#ifndef FBP_BLOB_H
#define FBP_BLOB_H

#include <ibase.h>
#include "php.h"

typedef struct firebird_blob {
    isc_blob_handle bl_handle;
    isc_db_handle *db_handle;
    isc_tr_handle *tr_handle;

    ISC_QUAD bl_id;
    ISC_LONG max_segment;
    ISC_LONG num_segments;
    ISC_LONG total_length;
    ISC_LONG type; // 0 - segmented, 1 - streamed
    ISC_LONG position;

    unsigned short is_writable;

    zend_object std;
} firebird_blob;

typedef struct firebird_blob_id {
    ISC_QUAD bl_id;
    zend_object std;
} firebird_blob_id;

void fbp_blob_ctor(firebird_blob *blob, isc_db_handle *db_handle, isc_tr_handle *tr_handle);
void fbp_blob_id_ctor(firebird_blob_id *blob_id, ISC_QUAD bl_id);
int fbp_blob_close(firebird_blob *blob);
int fbp_blob_get_info(firebird_blob *blob);
int fbp_blob_get(firebird_blob *blob, zval *return_value, size_t max_len);
int fbp_blob_put(firebird_blob *blob, const char *buf, size_t buf_size);
int fbp_blob_create(firebird_blob *blob);
int fbp_blob_open(firebird_blob *blob);

#endif /* FBP_BLOB_H */
