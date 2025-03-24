/*
   +----------------------------------------------------------------------+
   | PHP Version 7, 8                                                     |
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Jouni Ahto <jouni.ahto@exdec.fi>                            |
   |          Andrew Avdeev <andy@simgts.mv.ru>                           |
   |          Ard Biesheuvel <a.k.biesheuvel@its.tudelft.nl>              |
   |          Martin Koeditz <martin.koeditz@it-syn.de>                   |
   |          others                                                      |
   +----------------------------------------------------------------------+
   | You'll find history on Github                                        |
   | https://github.com/FirebirdSQL/php-firebird/commits/master           |
   +----------------------------------------------------------------------+
 */

#include <firebird/fb_c_api.h>
#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Blob_ce, *FireBird_Blob_Id_ce;
static zend_object_handlers FireBird_Blob_object_handlers, FireBird_Blob_Id_object_handlers;

// void blob_ctor(firebird_blob *blob, isc_db_handle *db_handle, isc_tr_handle *tr_handle)
// {
//     blob->bl_handle = 0;
//     blob->db_handle = db_handle;
//     blob->tr_handle = tr_handle;
// }

// void blob___construct(zval *blob, zval *transaction)
// {
//     object_init_ex(blob, FireBird_Blob_ce);
//     // OBJ_SET(FireBird_Blob_ce, blob, "transaction", transaction);

//     firebird_trans *tr = Z_TRANSACTION_P(transaction);

//     blob_ctor(Z_BLOB_P(blob), tr->db_handle, &tr->tr_handle);
// }

PHP_METHOD(Blob, __construct)
{
}

int blob_close(ISC_STATUS_ARRAY status, firebird_blob *blob)
{
    return isc_close_blob(status, &blob->bl_handle) ? FAILURE : SUCCESS;
}

PHP_METHOD(Blob, close)
{
    firebird_blob *blob = Z_BLOB_P(ZEND_THIS);
    ISC_STATUS_ARRAY status;

    if (blob_close(status, blob)) {
        update_err_props(status, FireBird_Blob_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(Blob, cancel)
{
    firebird_blob *blob = Z_BLOB_P(ZEND_THIS);
    ISC_STATUS_ARRAY status;

    if (isc_cancel_blob(status, &blob->bl_handle)) {
        update_err_props(status, FireBird_Blob_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

int blob_get_info(ISC_STATUS_ARRAY status, firebird_blob *blob)
{
    static char bl_items[] = {
        isc_info_blob_num_segments,
        isc_info_blob_max_segment,
        isc_info_blob_total_length,
        isc_info_blob_type
    };
    char bl_inf[sizeof(zend_long)*8] = {0};

    if (isc_blob_info(status, &blob->bl_handle, sizeof(bl_items), bl_items, sizeof(bl_inf), bl_inf)) {
        return FAILURE;
    }

    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* dpb;

    dpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_INFO_RESPONSE, bl_inf, sizeof(bl_inf));

    // FBDEBUG("Parsing BLOB info buffer");
    for (IXpbBuilder_rewind(dpb, st); !IXpbBuilder_isEof(dpb, st); IXpbBuilder_moveNext(dpb, st)) {
        unsigned char tag = IXpbBuilder_getTag(dpb, st);
        int val = IXpbBuilder_getInt(dpb, st);
        // FBDEBUG_NOFL(" tag: %d, val: %d", tag, val);

        switch(tag) {
            case isc_info_blob_num_segments:
                blob->num_segments = val;
                break;
            case isc_info_blob_max_segment:
                blob->max_segment = val;
                break;
            case isc_info_blob_total_length:
                blob->total_length = val;
                break;
            case isc_info_blob_type:
                blob->type = val;
                break;

            case isc_info_end:
                break;
            case isc_info_truncated:
                fbp_error("BLOB info buffer error: truncated");
                return FAILURE;
            case isc_info_error:
                fbp_error("BLOB info buffer error");
                return FAILURE;
            default:
                fbp_fatal("BUG! Unhandled BLOB info tag: %d", tag);
                return FAILURE;
        }
    }

    return SUCCESS;
}

void blob_update_props(zval *blob_o)
{
    firebird_blob *blob = Z_BLOB_P(blob_o);

    OBJ_SET_LONG(FireBird_Blob_ce, blob_o, "num_segments", blob->num_segments);
    OBJ_SET_LONG(FireBird_Blob_ce, blob_o, "max_segment", blob->max_segment);
    OBJ_SET_LONG(FireBird_Blob_ce, blob_o, "total_length", blob->total_length);
    OBJ_SET_LONG(FireBird_Blob_ce, blob_o, "type", blob->type);
    OBJ_SET_LONG(FireBird_Blob_ce, blob_o, "position", blob->position);
    OBJ_SET_LONG(FireBird_Blob_ce, blob_o, "is_writable", blob->is_writable);
}

int blob_get(ISC_STATUS_ARRAY status, firebird_blob *blob, zval *return_value, size_t max_len)
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

        stat = isc_get_segment(status, &blob->bl_handle, &seg_len, chunk_size, &ZSTR_VAL(bl_data)[cur_len]);

        if (status[0] == 1 && (stat != 0 && stat != isc_segstr_eof && stat != isc_segment)) {
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

PHP_METHOD(Blob, get)
{
    ISC_STATUS_ARRAY status;
    firebird_blob *blob = Z_BLOB_P(ZEND_THIS);
    zend_long max_len = 0;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(max_len)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == blob_get(status, blob, return_value, max_len)) {
        update_err_props(status, FireBird_Blob_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    blob_update_props(ZEND_THIS);
}

int blob_put(ISC_STATUS_ARRAY status, firebird_blob *blob, const char *buf, size_t buf_size)
{
    zend_ulong put_cnt = 0, rem_cnt;
    unsigned short chunk_size;

    for (rem_cnt = buf_size; rem_cnt > 0; rem_cnt -= chunk_size)  {
        chunk_size = rem_cnt > USHRT_MAX ? USHRT_MAX : (unsigned short)rem_cnt;
        if (isc_put_segment(status, &blob->bl_handle, chunk_size, &buf[put_cnt])) {
            return FAILURE;
        }
        put_cnt += chunk_size;
        blob->position += chunk_size;
    }

    return SUCCESS;
}

PHP_METHOD(Blob, put)
{
    ISC_STATUS_ARRAY status;
    firebird_blob *blob = Z_BLOB_P(ZEND_THIS);
    char *data;
    size_t data_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(data, data_len)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == blob_put(status, blob, data, data_len)) {
        update_err_props(status, FireBird_Blob_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    blob_update_props(ZEND_THIS);

    RETURN_TRUE;
}

const zend_function_entry FireBird_Blob_methods[] = {
    PHP_ME(Blob, __construct, arginfo_none, ZEND_ACC_PRIVATE | ZEND_ACC_CTOR)
    PHP_ME(Blob, close, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, cancel, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, get, arginfo_FireBird_Blob_get, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, put, arginfo_FireBird_Blob_put, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Blob_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Blob_create()");

    firebird_blob *s = zend_object_alloc(sizeof(firebird_blob), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Blob_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Blob_free_obj");

    firebird_blob *blob = Z_BLOB_O(obj);

    if (blob->bl_handle) {
        ISC_STATUS_ARRAY status;
        if (blob_close(status, blob)) {
            status_fbp_error(status);
        } else {
            blob->bl_handle = 0;
        }
    }

    zend_object_std_dtor(&blob->std);
}

void register_FireBird_Blob_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Blob", FireBird_Blob_methods);
    FireBird_Blob_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_ERR_PROPS(FireBird_Blob_ce);
    // DECLARE_PROP_OBJ(FireBird_Blob_ce, transaction, FireBird\\Transaction, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, num_segments, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, max_segment, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, total_length, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, type, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, position, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_BOOL(FireBird_Blob_ce, is_writable, ZEND_ACC_PROTECTED_SET);

    zend_class_implements(FireBird_Blob_ce, 1, FireBird_IError_ce);

    FireBird_Blob_ce->create_object = FireBird_Blob_create;
    FireBird_Blob_ce->default_object_handlers = &FireBird_Blob_object_handlers;

    memcpy(&FireBird_Blob_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Blob_object_handlers.offset = XtOffsetOf(firebird_blob, std);
    FireBird_Blob_object_handlers.free_obj = FireBird_Blob_free_obj;
}

void blob_id_ctor(firebird_blob_id *blob_id, ISC_QUAD bl_id)
{
    blob_id->bl_id = bl_id;
}

void blob_id___construct(zval *blob_id_o, ISC_QUAD bl_id)
{
    object_init_ex(blob_id_o, FireBird_Blob_Id_ce);
    blob_id_ctor(Z_BLOB_ID_P(blob_id_o), bl_id);
}

static zend_object *FireBird_Blob_Id_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Blob_Id_create()");

    firebird_blob_id *s = zend_object_alloc(sizeof(firebird_blob_id), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Blob_Id_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Blob_Id_free_obj");

    firebird_blob_id *blob_id = Z_BLOB_ID_O(obj);

    zend_object_std_dtor(&blob_id->std);
}

void register_FireBird_Blob_Id_ce()
{
    zend_class_entry tmp_ce;

    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Blob_Id", NULL);
    FireBird_Blob_Id_ce = zend_register_internal_class(&tmp_ce);

    FireBird_Blob_Id_ce->create_object = FireBird_Blob_Id_create;
    FireBird_Blob_Id_ce->default_object_handlers = &FireBird_Blob_Id_object_handlers;

    memcpy(&FireBird_Blob_Id_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Blob_Id_object_handlers.offset = XtOffsetOf(firebird_blob_id, std);
    FireBird_Blob_Id_object_handlers.free_obj = FireBird_Blob_Id_free_obj;
}

int blob_create(ISC_STATUS_ARRAY status, isc_db_handle *db_handle, isc_tr_handle *tr_handle, firebird_blob *blob)
{
    char bpb[] = { isc_bpb_version1, isc_bpb_type, 1, isc_bpb_type_stream };

    if (isc_create_blob2(status, db_handle, tr_handle, &blob->bl_handle, &blob->bl_id, sizeof(bpb), bpb)) {
        return FAILURE;
    }

    blob->is_writable = 1;

    return SUCCESS;
}

int blob_open(ISC_STATUS_ARRAY status, isc_db_handle *db_handle, isc_tr_handle *tr_handle, firebird_blob *blob)
{
    char bpb[] = { isc_bpb_version1, isc_bpb_type, 1, isc_bpb_type_stream };

    if (isc_open_blob2(status, db_handle, tr_handle, &blob->bl_handle, &blob->bl_id, sizeof(bpb), bpb)) {
        return FAILURE;
    }

    blob->is_writable = 0;

    // ISC_LONG new_pos;
    // status_exception::raise(Arg::Gds(isc_random) << "Seek mode must be 0 (START), 1 (CURRENT) or 2 (END)");
    // isc_seek_blob(status_vector, blob_handle, mode, offset, result);
    // if (isc_seek_blob(status, &blob->bl_handle, 0, 2, &new_pos)) {
    //     return FAILURE;
    // }
    // FBDEBUG("blob_seek: new_pos=%d", new_pos);

    return SUCCESS;
}
