#include <firebird/fb_c_api.h>
#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"

#include "fbp_blob.h"

fbp_object_accessor(firebird_blob);
fbp_object_accessor(firebird_blob_id);

void fbp_blob_ctor(firebird_blob *blob, isc_db_handle *db_handle, isc_tr_handle *tr_handle)
{
    blob->db_handle = db_handle;
    blob->tr_handle = tr_handle;
}

void fbp_blob_id_ctor(firebird_blob_id *blob_id, ISC_QUAD bl_id)
{
    blob_id->bl_id = bl_id;
}

int fbp_blob_close(firebird_blob *blob)
{
    return isc_close_blob(FBG(status), &blob->bl_handle) ? FAILURE : SUCCESS;
}

int fbp_blob_get_info(firebird_blob *blob)
{
    static char bl_items[] = {
        isc_info_blob_num_segments,
        isc_info_blob_max_segment,
        isc_info_blob_total_length,
        isc_info_blob_type
    };
    char bl_inf[sizeof(zend_long)*8] = {0};

    if (isc_blob_info(FBG(status), &blob->bl_handle, sizeof(bl_items), bl_items, sizeof(bl_inf), bl_inf)) {
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
                fbp_warning("BLOB info buffer error: truncated");
                return FAILURE;
            case isc_info_error:
                fbp_warning("BLOB info buffer error");
                return FAILURE;
            default:
                fbp_fatal("BUG! Unhandled BLOB info tag: %d", tag);
                return FAILURE;
        }
    }

    return SUCCESS;
}

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

int fbp_blob_create(firebird_blob *blob)
{
    char bpb[] = { isc_bpb_version1, isc_bpb_type, 1, isc_bpb_type_stream };

    if (isc_create_blob2(FBG(status), blob->db_handle, blob->tr_handle, &blob->bl_handle, &blob->bl_id, sizeof(bpb), bpb) ||
        fbp_blob_get_info(blob)) {
            return FAILURE;
    }

    blob->is_writable = 1;

    return SUCCESS;
}

int fbp_blob_open(firebird_blob *blob)
{
    char bpb[] = { isc_bpb_version1, isc_bpb_type, 1, isc_bpb_type_stream };

    if (isc_open_blob2(FBG(status), blob->db_handle, blob->tr_handle, &blob->bl_handle, &blob->bl_id, sizeof(bpb), bpb) ||
        fbp_blob_get_info(blob)) {
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
