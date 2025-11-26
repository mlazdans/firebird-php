#pragma once

#include <firebird/Interface.h>

#include "fbp/base.hpp"
#include "fbp/database.hpp"
#include "fbp/transaction.hpp"

#define BLOB_ID_LEN     18
#define BLOB_ID_MASK    "0x%" LL_MASK "x"

#define FBP_BLOB_TYPE_SEGMENTED isc_bpb_type_segmented
#define FBP_BLOB_TYPE_STREAMED  isc_bpb_type_stream

#define FBP_BLOB_SEEK_START   0
#define FBP_BLOB_SEEK_CURRENT 1
#define FBP_BLOB_SEEK_END     2

extern "C" {

#include <ibase.h>
#include "php.h"

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
    size_t dbh;
    size_t trh;
    size_t blh;
    firebird_blob_info *info;

    zend_object std;
} firebird_blob;

fbp_declare_object_accessor(firebird_blob);

typedef struct firebird_blob_id {
    ISC_QUAD bl_id;

    zend_object std;
} firebird_blob_id;


fbp_declare_object_accessor(firebird_blob_id);

extern zend_class_entry *FireBird_Blob_ce;
extern zend_class_entry *FireBird_Blob_Id_ce;

void register_FireBird_Blob_object_handlers();
void register_FireBird_Blob_Id_object_handlers();

int FireBird_Blob___construct(zval *self, zval *Transaction);
int FireBird_Blob_create(zval *self);
int FireBird_Blob_open(zval *self, zval *Blob_Id);
int FireBird_Blob_put(zval *self, const char *buf, unsigned int buf_size);
int FireBird_Blob_close(zval *self);
int FireBird_Blob_cancel(zval *self);
int FireBird_Blob_get(zval *self, zval *return_value, size_t max_len);

void FireBird_Blob_Id___construct(zval *self, ISC_QUAD bl_id);

int fbp_blob_id_to_string(ISC_QUAD const qd, size_t buf_len, char *buf);
int fbp_blob_id_to_quad(size_t id_len, char const *id, ISC_QUAD *qd);

}

using namespace Firebird;

namespace FBP {

class Blob: Base
{
private:
    Transaction &tra;
    IBlob *blob = nullptr;
    // ISC_QUAD id = {0};
    firebird_blob_info info = {0};

    void set_info();
public:
    Blob(Transaction &tra);
    ~Blob() noexcept;
    ISC_QUAD create();
    void open(ISC_QUAD *blob_id);
    zend_string *get_contents(ISC_LONG max_len);
    void put_contents(unsigned int buf_size, const char *buf);
    firebird_blob_info *get_info();
    void close();
    void cancel();
    void seek(int mode, int offset, int *new_offset);
    bool has_blob();
};

}
