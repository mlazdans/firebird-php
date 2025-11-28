#include "firebird_php.hpp"
#include "firebird_utils.h"

extern "C" {

#include "php.h"
// #include "php_firebird_includes.h"

fbp_object_accessor(firebird_blob);
fbp_object_accessor(firebird_blob_id);
static zend_object_handlers FireBird_Blob_object_handlers, FireBird_Blob_Id_object_handlers;

int FireBird_Blob___construct(zval *self, zval *Transaction)
{
    firebird_trans *tr = get_firebird_trans_from_zval(Transaction);
    firebird_blob *blob = get_firebird_blob_from_zval(self);

    size_t blh;
    const firebird_blob_info *info;

    if (fbu_blob_init(tr->dbh, tr->trh, &blh)) {
        return FAILURE;
    }

    if (fbu_blob_get_info(tr->dbh, blh, &info)) {
        return FAILURE;
    }

    blob->blh = blh;
    blob->dbh = tr->dbh;
    blob->trh = tr->trh;
    blob->info = info;

    PROP_SET(FireBird_Blob_ce, self, "transaction", Transaction);

    return SUCCESS;
}

PHP_METHOD(FireBird_Blob, __construct)
{
    zval *Transaction;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Transaction, FireBird_Transaction_ce)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Blob___construct(ZEND_THIS, Transaction)) {
        RETURN_THROWS();
    }
}

int FireBird_Blob_close(zval *self)
{
    firebird_blob *blob = get_firebird_blob_from_zval(self);

    if (fbu_blob_close(blob->dbh, blob->blh)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Blob, close)
{
    if (FireBird_Blob_close(ZEND_THIS)) {
        RETURN_THROWS();
    }
}

int FireBird_Blob_cancel(zval *self)
{
    firebird_blob *blob = get_firebird_blob_from_zval(self);

    if (fbu_blob_cancel(blob->dbh, blob->blh)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Blob, cancel)
{
    if (FireBird_Blob_cancel(ZEND_THIS)) {
        RETURN_THROWS();
    }
}

int FireBird_Blob_get(zval *self, zval *return_value, size_t max_len)
{
    firebird_blob *blob = get_firebird_blob_from_zval(self);
    zend_string *data;

    if (fbu_blob_get(blob->dbh, blob->blh, max_len, &data)) {
        return FAILURE;
    }

    ZVAL_STR(return_value, data);

    return SUCCESS;
}

PHP_METHOD(FireBird_Blob, get)
{
    zend_long max_len = 0;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(max_len)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Blob_get(ZEND_THIS, return_value, max_len)) {
        RETURN_THROWS();
    }
}

int FireBird_Blob_put(zval *self, const char *buf, unsigned int buf_size)
{
    firebird_blob *blob = get_firebird_blob_from_zval(self);

    if (fbu_blob_put(blob->dbh, blob->blh, buf_size, buf)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Blob, put)
{
    char *data;
    size_t data_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(data, data_len)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Blob_put(ZEND_THIS, data, data_len)) {
        RETURN_THROWS();
    }
}

int FireBird_Blob_open(zval *self, zval *Blob_Id)
{
    firebird_blob *blob = get_firebird_blob_from_zval(self);
    firebird_blob_id *id = get_firebird_blob_id_from_zval(Blob_Id);

    if (fbu_blob_open(blob->dbh, blob->blh, &id->bl_id)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Blob, open)
{
    zval *blob_id;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(blob_id, FireBird_Blob_Id_ce)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Blob_open(ZEND_THIS, blob_id)) {
        RETURN_THROWS();
    }
}

int FireBird_Blob_create(zval *self)
{
    firebird_blob *blob = get_firebird_blob_from_zval(self);

    if (fbu_blob_create(blob->dbh, blob->blh)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Blob, create)
{
    if (FireBird_Blob_create(ZEND_THIS)) {
        RETURN_THROWS();
    }
}

int FireBird_Blob_seek(zval *self, int mode, int offset, int *new_offset)
{
    firebird_blob *blob = get_firebird_blob_from_zval(self);

    if (fbu_blob_seek(blob->dbh, blob->blh, mode, offset, new_offset)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Blob, seek)
{
    zend_long offset, mode;
    ISC_LONG new_offset = 0;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(offset)
        Z_PARAM_LONG(mode)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Blob_seek(ZEND_THIS, mode, offset, &new_offset)) {
        RETURN_THROWS();
    }

    RETURN_LONG(new_offset);
}

static zend_object *FireBird_Blob_create_object(zend_class_entry *ce)
{
    firebird_blob *blob = (firebird_blob *)zend_object_alloc(sizeof(firebird_blob), ce);

    FBDEBUG("%s(blob=%p)", __func__, blob);

    zend_object_std_init(&blob->std, ce);
    object_properties_init(&blob->std, ce);

    return &blob->std;
}

static void FireBird_Blob_free_obj(zend_object *obj)
{
    firebird_blob *blob = get_firebird_blob_from_obj(obj);

    FBDEBUG("~%s(blob=%p)", __func__, blob);

    if (fbu_blob_free(blob->dbh, blob->blh)) {
        // TODO: throws?
    }

    blob->blh = 0;

    zend_object_std_dtor(&blob->std);
}

static zend_object *FireBird_Blob_Id_create_object(zend_class_entry *ce)
{
    FBDEBUG("++ %s", __func__);

    firebird_blob_id *s = (firebird_blob_id *)zend_object_alloc(sizeof(firebird_blob_id), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Blob_Id_free_obj(zend_object *obj)
{
    firebird_blob_id *blob_id = get_firebird_blob_id_from_obj(obj);

    FBDEBUG("~%s(blob_id=%p)", __func__, blob_id);

    zend_object_std_dtor(&blob_id->std);
}

void FireBird_Blob_Id___construct(zval *self, ISC_QUAD bl_id)
{
    firebird_blob_id *blob_id = get_firebird_blob_id_from_zval(self);
    blob_id->bl_id = bl_id;
}

PHP_METHOD(FireBird_Blob_Id, __construct)
{
}

PHP_METHOD(FireBird_Blob_Id, to_legacy_id)
{
    zval *Blob_Id;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Blob_Id, FireBird_Blob_Id_ce)
    ZEND_PARSE_PARAMETERS_END();

    char buf[BLOB_ID_LEN+1];
    firebird_blob_id *blob_id = get_firebird_blob_id_from_zval(Blob_Id);

    int size = fbp_blob_id_to_string(blob_id->bl_id, sizeof(buf), buf);

    RETURN_STRINGL(buf, size);
}

PHP_METHOD(FireBird_Blob_Id, from_legacy_id)
{
    char *id;
    size_t id_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(id, id_len)
    ZEND_PARSE_PARAMETERS_END();

    char buf[BLOB_ID_LEN+1];

    object_init_ex(return_value, FireBird_Blob_Id_ce);
    firebird_blob_id *blob_id = get_firebird_blob_id_from_zval(return_value);

    // TODO: this actually does not throw. Add throwing exception.
    if (fbp_blob_id_to_quad(id_len, id, &blob_id->bl_id)) {
        RETURN_THROWS();
    }
}

static zval* FireBird_Blob_read_property(zend_object *obj, zend_string *name, int type,
    void **cache_slot, zval *rv)
{
    firebird_blob *blob = get_firebird_blob_from_obj(obj);

    if (zend_string_equals_literal(name, "num_segments")) {
        ZVAL_LONG(rv, blob->info->num_segments);
        return rv;
    }
    if (zend_string_equals_literal(name, "max_segment")) {
        ZVAL_LONG(rv, blob->info->max_segment);
        return rv;
    }
    if (zend_string_equals_literal(name, "total_length")) {
        ZVAL_LONG(rv, blob->info->total_length);
        return rv;
    }
    if (zend_string_equals_literal(name, "type")) {
        ZVAL_LONG(rv, blob->info->type);
        return rv;
    }
    if (zend_string_equals_literal(name, "position")) {
        ZVAL_LONG(rv, blob->info->position);
        return rv;
    }
    if (zend_string_equals_literal(name, "is_writable")) {
        ZVAL_BOOL(rv, blob->info->is_writable);
        return rv;
    }

    return zend_std_read_property(obj, name, type, cache_slot, rv);
}


void register_FireBird_Blob_object_handlers()
{
    FireBird_Blob_ce->create_object = FireBird_Blob_create_object;
    FireBird_Blob_ce->default_object_handlers = &FireBird_Blob_object_handlers;

    memcpy(&FireBird_Blob_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Blob_object_handlers.offset = XtOffsetOf(firebird_blob, std);
    FireBird_Blob_object_handlers.free_obj = FireBird_Blob_free_obj;
    FireBird_Blob_object_handlers.read_property = FireBird_Blob_read_property;
}

void register_FireBird_Blob_Id_object_handlers()
{
    FireBird_Blob_Id_ce->create_object = FireBird_Blob_Id_create_object;
    FireBird_Blob_Id_ce->default_object_handlers = &FireBird_Blob_Id_object_handlers;

    memcpy(&FireBird_Blob_Id_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Blob_Id_object_handlers.offset = XtOffsetOf(firebird_blob_id, std);
    FireBird_Blob_Id_object_handlers.free_obj = FireBird_Blob_Id_free_obj;
}

int fbp_blob_id_to_string(ISC_QUAD const qd, size_t buf_len, char *buf)
{
    /* shortcut for most common case */
    if (sizeof(ISC_QUAD) == sizeof(ISC_UINT64)) {
        return slprintf(buf, buf_len, "0x%0*" LL_MASK "x", 16, *(ISC_UINT64*)(void *) &qd);
    } else {
        ISC_UINT64 res = ((ISC_UINT64) qd.gds_quad_high << 0x20) | qd.gds_quad_low;
        return slprintf(buf, buf_len, "0x%0*" LL_MASK "x", 16, res);
    }
}

int fbp_blob_id_to_quad(size_t id_len, char const *id, ISC_QUAD *qd)
{
    /* shortcut for most common case */
    if (sizeof(ISC_QUAD) == sizeof(ISC_UINT64)) {
        return sscanf(id, BLOB_ID_MASK, (ISC_UINT64 *) qd) > 0 ? SUCCESS : FAILURE;
    } else {
        ISC_UINT64 res;
        if (sscanf(id, BLOB_ID_MASK, &res) > 0) {
            qd->gds_quad_high = (ISC_LONG) (res >> 0x20);
            qd->gds_quad_low = (ISC_LONG) (res & 0xFFFFFFFF);
            return SUCCESS;
        }
        return FAILURE;
    }
}

#if 0
// NOTE: chunked get - keep for now!
int fbp_blob_get(firebird_blob *blob, zval *return_value, size_t max_len)
{
    ISC_STATUS stat;
    zend_string *bl_data;
    size_t cur_len, remaining_len;
    unsigned short seg_len;

    remaining_len = blob->total_length - blob->position;
    FBDEBUG_NOFL("  remaining_len: %d", remaining_len);
    if (!max_len || max_len > remaining_len) {
        max_len = remaining_len;
    }

    bl_data = zend_string_safe_alloc(1, max_len, 0, 0);

    for (cur_len = stat = 0; (stat == 0 || stat == isc_segment) && cur_len < max_len; cur_len += seg_len) {

        unsigned short chunk_size = (max_len-cur_len) > USHRT_MAX ? USHRT_MAX
            : (unsigned short)(max_len-cur_len);

        stat = isc_get_segment(FBG(status), &blob->bl_handle, &seg_len, chunk_size, &ZSTR_VAL(bl_data)[cur_len]);

        if (FBG(status)[0] == 1 && (stat != 0 && stat != isc_segstr_eof && stat != isc_segment)) {
            zend_string_free(bl_data);
            return FAILURE;
        }

        blob->position += chunk_size;
    }

    ZSTR_VAL(bl_data)[cur_len] = '\0';
    ZSTR_LEN(bl_data) = cur_len;
    RETVAL_NEW_STR(bl_data);

    return SUCCESS;
}
#endif

#if 0
// NOTE: chunked put - keep for now!
int fbp_blob_put(firebird_blob *blob, const char *buf, size_t buf_size)
{
    zend_ulong put_cnt = 0, rem_cnt;
    unsigned short chunk_size;

    for (rem_cnt = buf_size; rem_cnt > 0; rem_cnt -= chunk_size)  {
        chunk_size = rem_cnt > USHRT_MAX ? USHRT_MAX : (unsigned short)rem_cnt;
        if (isc_put_segment(FBG(status), &blob->bl_handle, chunk_size, &buf[put_cnt])) {
            return FAILURE;
        }
        put_cnt += chunk_size;
    }

    blob->position += put_cnt;
    blob->total_length += put_cnt;

    return SUCCESS;
}
#endif

}
