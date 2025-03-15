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


// ibase_blob_add    - Add data into a newly created blob
// ibase_blob_cancel - Cancel creating blob
// ibase_blob_close  - Close blob
// ibase_blob_create - Create a new blob for adding data
// ibase_blob_echo   - Output blob contents to browser
// ibase_blob_get    - Get len bytes data from open blob
// ibase_blob_import - Create blob, copy file in it, and close it
// ibase_blob_info   - Return blob length and other useful info
// ibase_blob_open   - Open blob for retrieving data parts

#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Blob_ce;
static zend_object_handlers FireBird_Blob_object_handlers;

#define BLOB_CLOSE  1
#define BLOB_CANCEL 2

void blob_ctor(zval *blob_o, zval *transaction)
{
    zend_update_property(FireBird_Blob_ce, O_SET(blob_o, transaction));

    firebird_blob *blob = Z_BLOB_P(blob_o);
    firebird_trans *tr = Z_TRANSACTION_P(transaction);

    blob->db_handle = tr->db_handle;
    blob->tr_handle = &tr->tr_handle;
}

static void _php_firebird_free_blob(zend_resource *rsrc, ISC_STATUS_ARRAY status)
{
    firebird_blob *ib_blob = (firebird_blob *)rsrc->ptr;

    if (ib_blob->bl_handle != 0) { /* blob open*/
        if (isc_cancel_blob(status, &ib_blob->bl_handle)) {
            _php_firebird_module_error("You can lose data. Close any blob after reading from or "
                "writing to it. Use firebird_blob_close() before calling ibase_close()");
        }
    }
    efree(ib_blob);
}

int _php_firebird_string_to_quad(char const *id, ISC_QUAD *qd)
{
    /* shortcut for most common case */
    if (sizeof(ISC_QUAD) == sizeof(ISC_UINT64)) {
        return sscanf(id, BLOB_ID_MASK, (ISC_UINT64 *) qd);
    } else {
        ISC_UINT64 res;
        if (sscanf(id, BLOB_ID_MASK, &res)) {
            qd->gds_quad_high = (ISC_LONG) (res >> 0x20);
            qd->gds_quad_low = (ISC_LONG) (res & 0xFFFFFFFF);
            return 1;
        }
        return 0;
    }
}

zend_string *_php_firebird_quad_to_string(ISC_QUAD const qd)
{
    /* shortcut for most common case */
    if (sizeof(ISC_QUAD) == sizeof(ISC_UINT64)) {
        return strpprintf(BLOB_ID_LEN+1, "0x%0*" LL_MASK "x", 16, *(ISC_UINT64*)(void *) &qd);
    } else {
        ISC_UINT64 res = ((ISC_UINT64) qd.gds_quad_high << 0x20) | qd.gds_quad_low;
        return strpprintf(BLOB_ID_LEN+1, "0x%0*" LL_MASK "x", 16, res);
    }
}

int _php_firebird_blob_get(ISC_STATUS_ARRAY status, zval *return_value, firebird_blob *ib_blob, zend_ulong max_len)
{
    if (ib_blob->bl_id.gds_quad_high || ib_blob->bl_id.gds_quad_low) { /*not null ?*/

        ISC_STATUS stat;
        zend_string *bl_data;
        zend_ulong cur_len;
        unsigned short seg_len;

        bl_data = zend_string_safe_alloc(1, max_len, 0, 0);

        for (cur_len = stat = 0; (stat == 0 || stat == isc_segment) && cur_len < max_len; cur_len += seg_len) {

            unsigned short chunk_size = (max_len-cur_len) > USHRT_MAX ? USHRT_MAX
                : (unsigned short)(max_len-cur_len);

            stat = isc_get_segment(status, &ib_blob->bl_handle, &seg_len, chunk_size, &ZSTR_VAL(bl_data)[cur_len]);
        }

        if (status[0] == 1 && (stat != 0 && stat != isc_segstr_eof && stat != isc_segment)) {
            zend_string_free(bl_data);
            return FAILURE;
        }
        ZSTR_VAL(bl_data)[cur_len] = '\0';
        ZSTR_LEN(bl_data) = cur_len;
        RETVAL_NEW_STR(bl_data);
    } else { /* null blob */
        RETVAL_EMPTY_STRING(); /* empty string */
    }
    return SUCCESS;
}

int _php_firebird_blob_add(ISC_STATUS_ARRAY status, zval *string_arg, firebird_blob *ib_blob)
{
    zend_ulong put_cnt = 0, rem_cnt;
    unsigned short chunk_size;

    convert_to_string_ex(string_arg);

    for (rem_cnt = Z_STRLEN_P(string_arg); rem_cnt > 0; rem_cnt -= chunk_size)  {

        chunk_size = rem_cnt > USHRT_MAX ? USHRT_MAX : (unsigned short)rem_cnt;

        if (isc_put_segment(status, &ib_blob->bl_handle, chunk_size, &Z_STRVAL_P(string_arg)[put_cnt] )) {
            return FAILURE;
        }
        put_cnt += chunk_size;
    }
    return SUCCESS;
}

static int _php_firebird_blob_info(isc_blob_handle bl_handle, firebird_blobinfo *bl_info, ISC_STATUS_ARRAY status)
{
    static char bl_items[] = {
        isc_info_blob_num_segments,
        isc_info_blob_max_segment,
        isc_info_blob_total_length,
        isc_info_blob_type
    };

    char bl_inf[sizeof(zend_long)*8], *p;

    bl_info->max_segment = 0;
    bl_info->num_segments = 0;
    bl_info->total_length = 0;
    bl_info->bl_stream = 0;

    if (isc_blob_info(status, &bl_handle, sizeof(bl_items), bl_items, sizeof(bl_inf), bl_inf)) {
        return FAILURE;
    }

    for (p = bl_inf; *p != isc_info_end && p < bl_inf + sizeof(bl_inf);) {
        unsigned short item_len;
        int item = *p++;

        item_len = (short) isc_vax_integer(p, 2);
        p += 2;
        switch (item) {
            case isc_info_blob_num_segments:
                bl_info->num_segments = isc_vax_integer(p, item_len); // TODO: isc_portable_integer
                break;
            case isc_info_blob_max_segment:
                bl_info->max_segment = isc_vax_integer(p, item_len);
                break;
            case isc_info_blob_total_length:
                bl_info->total_length = isc_vax_integer(p, item_len);
                break;
            case isc_info_blob_type:
                bl_info->bl_stream = isc_vax_integer(p, item_len);
                break;
            case isc_info_end:
                break;
            case isc_info_truncated:
            case isc_info_error:  /* hmm. don't think so...*/
                _php_firebird_module_error("PHP module internal error");
                return FAILURE;
        } /* switch */
        p += item_len;
    } /* for */
    return SUCCESS;
}

// static void _php_firebird_blob_end(INTERNAL_FUNCTION_PARAMETERS, int bl_end, ISC_STATUS_ARRAY status)
// {
//     zval *blob_arg;
//     firebird_blob *ib_blob;

//     if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "r", &blob_arg)) {
//         return;
//     }

//     ib_blob = (firebird_blob *)zend_fetch_resource_ex(blob_arg, "Interbase blob", le_blob);

//     if (bl_end == BLOB_CLOSE) { /* return id here */

//         if (ib_blob->bl_id.gds_quad_high || ib_blob->bl_id.gds_quad_low) { /*not null ?*/
//             if (isc_close_blob(status, &ib_blob->bl_handle)) {
//                 RETURN_FALSE;
//             }
//         }
//         ib_blob->bl_handle = 0;

//         RETVAL_NEW_STR(_php_firebird_quad_to_string(ib_blob->bl_id));
//     } else { /* discard created blob */
//         if (isc_cancel_blob(status, &ib_blob->bl_handle)) {
//             RETURN_FALSE;
//         }
//         ib_blob->bl_handle = 0;
//         RETVAL_TRUE;
//     }
//     zend_list_delete(Z_RES_P(blob_arg));
// }

PHP_METHOD(Blob, __construct)
{
}

PHP_METHOD(Blob, close)
{
    firebird_blob *blob = Z_BLOB_P(ZEND_THIS);
    ISC_STATUS_ARRAY status;

    if (isc_close_blob(status, &blob->bl_handle)) {
        update_err_props(status, FireBird_Blob_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

const zend_function_entry FireBird_Blob_methods[] = {
    PHP_ME(Blob, __construct, arginfo_none, ZEND_ACC_PRIVATE)
    PHP_ME(Blob, close, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
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
        if (isc_close_blob(status, &blob->bl_handle)) {
            // TODO: report errors?
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

    DECLARE_PROP_OBJ(FireBird_Blob_ce, transaction, FireBird\\Transaction, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Blob_ce);

    zend_class_implements(FireBird_Blob_ce, 1, FireBird_IError_ce);

    FireBird_Blob_ce->create_object = FireBird_Blob_create;
    FireBird_Blob_ce->default_object_handlers = &FireBird_Blob_object_handlers;

    memcpy(&FireBird_Blob_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Blob_object_handlers.offset = XtOffsetOf(firebird_blob, std);
    FireBird_Blob_object_handlers.free_obj = FireBird_Blob_free_obj;
}
