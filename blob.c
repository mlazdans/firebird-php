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

#include "blob.h"
#include "fbp_blob.h"

#include "transaction.h"
#include "fbp_transaction.h"

zend_class_entry *FireBird_Blob_ce;
zend_class_entry *FireBird_Blob_Id_ce;

static zend_object_handlers FireBird_Blob_object_handlers, FireBird_Blob_Id_object_handlers;

static void _FireBird_Blob_update_props(zval *Blob)
{
    firebird_blob *blob = get_firebird_blob_from_zval(Blob);

    OBJ_SET_LONG(FireBird_Blob_ce, Blob, "num_segments", blob->num_segments);
    OBJ_SET_LONG(FireBird_Blob_ce, Blob, "max_segment", blob->max_segment);
    OBJ_SET_LONG(FireBird_Blob_ce, Blob, "total_length", blob->total_length);
    OBJ_SET_LONG(FireBird_Blob_ce, Blob, "type", blob->type);
    OBJ_SET_LONG(FireBird_Blob_ce, Blob, "position", blob->position);
    OBJ_SET_LONG(FireBird_Blob_ce, Blob, "is_writable", blob->is_writable);
}

void FireBird_Blob___construct(zval *Blob, zval *Transaction)
{
    firebird_trans *tr = get_firebird_trans_from_zval(Transaction);
    firebird_blob *blob = get_firebird_blob_from_zval(Blob);

    fbp_blob_ctor(blob, tr->db_handle, tr->tr_handle);

    OBJ_SET(FireBird_Blob_ce, Blob, "transaction", Transaction);
}

PHP_METHOD(Blob, __construct)
{
    zval *Transaction;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Transaction, FireBird_Transaction_ce)
    ZEND_PARSE_PARAMETERS_END();

    FireBird_Blob___construct(ZEND_THIS, Transaction);
}

int FireBird_Blob_close(zval *Blob)
{
    firebird_blob *blob = get_firebird_blob_from_zval(Blob);

    if (fbp_blob_close(blob)) {
        update_err_props(FBG(status), FireBird_Blob_ce, Blob);
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(Blob, close)
{
    RETVAL_BOOL(SUCCESS == FireBird_Blob_close(ZEND_THIS));
}

int FireBird_Blob_cancel(zval *Blob)
{
    firebird_blob *blob = get_firebird_blob_from_zval(Blob);

    if (isc_cancel_blob(FBG(status), &blob->bl_handle)) {
        update_err_props(FBG(status), FireBird_Blob_ce, Blob);
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(Blob, cancel)
{
    RETVAL_BOOL(SUCCESS == FireBird_Blob_cancel(ZEND_THIS));
}

int FireBird_Blob_get(zval *Blob, zval *return_value, size_t max_len)
{
    firebird_blob *blob = get_firebird_blob_from_zval(Blob);

    if (fbp_blob_get(blob, return_value, max_len)) {
        update_err_props(FBG(status), FireBird_Blob_ce, Blob);
        return FAILURE;
    }

    _FireBird_Blob_update_props(Blob);

    return SUCCESS;
}

PHP_METHOD(Blob, get)
{
    zend_long max_len = 0;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(max_len)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Blob_get(ZEND_THIS, return_value, max_len)) {
        RETURN_FALSE;
    }
}

int FireBird_Blob_put(zval *Blob, const char *buf, size_t buf_size)
{
    firebird_blob *blob = get_firebird_blob_from_zval(Blob);

    if (fbp_blob_put(blob, buf, buf_size)) {
        update_err_props(FBG(status), FireBird_Blob_ce, Blob);
        return FAILURE;
    }

    _FireBird_Blob_update_props(Blob);

    return SUCCESS;
}

PHP_METHOD(Blob, put)
{
    char *data;
    size_t data_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(data, data_len)
    ZEND_PARSE_PARAMETERS_END();

    RETVAL_BOOL(SUCCESS == FireBird_Blob_put(ZEND_THIS, data, data_len));
}

int FireBird_Blob_open(zval *Blob, zval *Blob_Id)
{
    firebird_blob *blob = get_firebird_blob_from_zval(Blob);
    firebird_blob_id *blob_id = get_firebird_blob_id_from_zval(Blob_Id);

    blob->bl_id = blob_id->bl_id;

    if (fbp_blob_open(blob)) {
        update_err_props(FBG(status), FireBird_Blob_ce, Blob);
        return FAILURE;
    }

    _FireBird_Blob_update_props(Blob);

    return SUCCESS;
}

PHP_METHOD(Blob, open)
{
    zval *Blob_Id;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Blob_Id, FireBird_Blob_Id_ce)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Blob_open(ZEND_THIS, Blob_Id)) {
        update_err_props(FBG(status), FireBird_Blob_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

int FireBird_Blob_create(zval *Blob)
{
    firebird_blob *blob = get_firebird_blob_from_zval(Blob);

    if (fbp_blob_create(blob)) {
        update_err_props(FBG(status), FireBird_Blob_ce, Blob);
        return FAILURE;
    }

    _FireBird_Blob_update_props(Blob);

    return SUCCESS;
}

PHP_METHOD(Blob, create)
{
    RETVAL_BOOL(SUCCESS == FireBird_Blob_create(ZEND_THIS));
}

PHP_METHOD(Blob, seek)
{
    zend_long pos, mode;
    ISC_LONG new_pos = 0;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(pos)
        Z_PARAM_LONG(mode)
    ZEND_PARSE_PARAMETERS_END();

    firebird_blob *blob = get_firebird_blob_from_zval(ZEND_THIS);

    if (fbp_blob_seek(blob, pos, mode, &new_pos)) {
        update_err_props(FBG(status), FireBird_Blob_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    _FireBird_Blob_update_props(ZEND_THIS);

    RETURN_LONG(new_pos);
}

const zend_function_entry FireBird_Blob_methods[] = {
    PHP_ME(Blob, __construct, arginfo_FireBird_Blob___construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Blob, create, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, open, arginfo_FireBird_Blob_open, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, close, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, cancel, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, get, arginfo_FireBird_Blob_get, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, put, arginfo_FireBird_Blob_put, ZEND_ACC_PUBLIC)
    PHP_ME(Blob, seek, arginfo_FireBird_Blob_seek, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *new_FireBird_Blob(zend_class_entry *ce)
{
    FBDEBUG("new_FireBird_Blob()");

    firebird_blob *s = zend_object_alloc(sizeof(firebird_blob), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void free_FireBird_Blob(zend_object *obj)
{
    FBDEBUG("free_FireBird_Blob");

    firebird_blob *blob = get_firebird_blob_from_obj(obj);

    if (blob->bl_handle) {
        if (fbp_blob_close(blob)) {
            fbp_status_error(FBG(status));
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
    DECLARE_PROP_OBJ(FireBird_Blob_ce, transaction, FireBird\\Transaction, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, num_segments, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, max_segment, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, total_length, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, type, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Blob_ce, position, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_BOOL(FireBird_Blob_ce, is_writable, ZEND_ACC_PROTECTED_SET);

    zend_class_implements(FireBird_Blob_ce, 1, FireBird_IError_ce);

    FireBird_Blob_ce->create_object = new_FireBird_Blob;
    FireBird_Blob_ce->default_object_handlers = &FireBird_Blob_object_handlers;

    memcpy(&FireBird_Blob_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Blob_object_handlers.offset = XtOffsetOf(firebird_blob, std);
    FireBird_Blob_object_handlers.free_obj = free_FireBird_Blob;
}

void FireBird_Blob_Id___construct(zval *Blob_Id, ISC_QUAD bl_id)
{
    fbp_blob_id_ctor(get_firebird_blob_id_from_zval(Blob_Id), bl_id);
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

    firebird_blob_id *blob_id = get_firebird_blob_id_from_obj(obj);

    zend_object_std_dtor(&blob_id->std);
}

PHP_METHOD(Blob_Id, __construct)
{
}

PHP_METHOD(Blob_Id, to_legacy_id)
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

PHP_METHOD(Blob_Id, from_legacy_id)
{
    char *id;
    size_t id_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(id, id_len)
    ZEND_PARSE_PARAMETERS_END();

    char buf[BLOB_ID_LEN+1];

    object_init_ex(return_value, FireBird_Blob_Id_ce);
    firebird_blob_id *blob_id = get_firebird_blob_id_from_zval(return_value);

    if (fbp_blob_id_to_quad(id_len, id, &blob_id->bl_id)) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

const zend_function_entry FireBird_Blob_Id_methods[] = {
    PHP_ME(Blob_Id, __construct, arginfo_none, ZEND_ACC_PRIVATE | ZEND_ACC_CTOR)
    PHP_ME(Blob_Id, to_legacy_id, arginfo_FireBird_Blob_Id_to_legacy_id, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(Blob_Id, from_legacy_id, arginfo_FireBird_Blob_Id_from_legacy_id, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};

void register_FireBird_Blob_Id_ce()
{
    zend_class_entry tmp_ce;

    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Blob_Id", FireBird_Blob_Id_methods);
    FireBird_Blob_Id_ce = zend_register_internal_class(&tmp_ce);

    FireBird_Blob_Id_ce->create_object = FireBird_Blob_Id_create;
    FireBird_Blob_Id_ce->default_object_handlers = &FireBird_Blob_Id_object_handlers;

    memcpy(&FireBird_Blob_Id_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Blob_Id_object_handlers.offset = XtOffsetOf(firebird_blob_id, std);
    FireBird_Blob_Id_object_handlers.free_obj = FireBird_Blob_Id_free_obj;
}
