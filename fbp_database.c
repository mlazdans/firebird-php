#include <firebird/fb_c_api.h>
#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"

#include "database.h"
#include "fbp_database.h"

fbp_object_accessor(firebird_db);

/**
 * zval* Args intanceof Create_Args|Connect_Args
 */
int fbp_database_build_dpb(zend_class_entry *ce, zval *Args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written)
{
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_DPB, NULL, 0);

    zend_property_info *prop_info = NULL;
    zend_string *prop_name = NULL;
    zval rv, *val, *checkval;
    int i;

    xpb_insert_tag(isc_dpb_version2);
    xpb_insert_int(isc_dpb_sql_dialect, SQL_DIALECT_CURRENT);

    xpb_insert_zmap(ce, Args, xpb_zmap, xpb, st);

#if FB_API_VER >= 40
    // Do not handle directly INT128 or DECFLOAT, convert to VARCHAR at server instead
    xpb_insert_string(isc_dpb_set_bind, "int128 to varchar;decfloat to varchar");
#endif

    *num_dpb_written = IXpbBuilder_getBufferLength(xpb, st);
    *dpb_buf = IXpbBuilder_getBuffer(xpb, st);

    // Needed? Not needed?
    // IXpbBuilder_dispose(xpb);
    // IStatus_dispose(st);

    return SUCCESS;
}

int fbp_database_connect(firebird_db *db, zval *Connect_Args)
{
    zval rv, *database;
    const char *dpb_buffer;
    short num_dpb_written;

    if (FAILURE == fbp_database_build_dpb(FireBird_Connect_Args_ce, Connect_Args, &database_connect_zmap, &dpb_buffer, &num_dpb_written)) {
        return FAILURE;
    }

    database = OBJ_GET(FireBird_Connect_Args_ce, Connect_Args, "database", &rv);

    FBDEBUG("Database::connect: %s", Z_STRVAL_P(database));
    if (isc_attach_database(FBG(status), (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &db->db_handle, num_dpb_written, dpb_buffer)) {
        return FAILURE;
    }
    FBDEBUG_NOFL("  connected, handle: %d", db->db_handle);

    return SUCCESS;
}

int fbp_database_create(firebird_db *db, zval *Create_Args)
{
    zval rv, *database;
    const char *dpb_buffer;
    short num_dpb_written;

    if (FAILURE == fbp_database_build_dpb(FireBird_Create_Args_ce, Create_Args, &database_create_zmap, &dpb_buffer, &num_dpb_written)) {
        return FAILURE;
    }

    database = OBJ_GET(FireBird_Create_Args_ce, Create_Args, "database", &rv);

    // if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
    //     zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
    //     return FAILURE;
    // }

    FBDEBUG("database_create: %s", Z_STRVAL_P(database));
    if (isc_create_database(FBG(status), (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &db->db_handle, num_dpb_written, dpb_buffer, 0)) {
        return FAILURE;
    }
    FBDEBUG("Created, handle: %d", db->db_handle);

    return SUCCESS;
}

int fbp_database_get_info(firebird_db *db, zval *Db_Info,
    size_t info_req_size, char *info_req, size_t info_resp_size, char *info_resp, size_t max_limbo_count)
{
    if (isc_database_info(FBG(status), &db->db_handle, info_req_size, info_req, info_resp_size, info_resp)) {
        return FAILURE;
    }

    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* dpb;

    dpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_INFO_RESPONSE, info_resp, info_resp_size);

    const char *str;
    ISC_INT64 len;
    size_t limbo_count = 0;
    zval info_limbo;

    array_init(&info_limbo);

#define READ_BIGINT(item) READ_BIGINT_(item, isc)
#define READ_BIGINT_FB(item) READ_BIGINT_(item, fb)

#define READ_STR(item) READ_STR_(item, isc)
#define READ_STR_FB(item) READ_STR_(item, fb)

#define READ_BIGINT_(item, pref)                                         \
    case pref ## _info_ ## item:                                         \
        zend_update_property_long(FireBird_Db_Info_ce, Z_OBJ_P(Db_Info), \
            #item, sizeof(#item) - 1, IXpbBuilder_getBigInt(dpb, st));   \
        break

#define READ_STR_(item, pref)                                               \
    case pref ## _info_ ## item:                                            \
        zend_update_property_stringl(FireBird_Db_Info_ce, Z_OBJ_P(Db_Info), \
            #item, sizeof(#item) - 1, b, len);                              \
        break

// b[0] holds count of entries, at the moment = 1
// b[1] holds str length
// b[2] holds pointer to str (not null terminated)
#define READ_STR_ARRAY(item)                                        \
    case isc_info_##item: {                                         \
        ISC_UCHAR count = *b++;                                     \
        ISC_UCHAR str_len;                                          \
        zval str_arr;                                               \
        array_init(&str_arr);                                       \
        FBDEBUG_NOFL("  count: %d", count);                         \
        for (int i = 0; i < count; i++) {                           \
            str_len = *b++;                                         \
            FBDEBUG_NOFL("  item: %.*s", str_len, b);               \
            add_next_index_stringl(&str_arr, b, str_len);           \
            b += str_len;                                           \
        }                                                           \
        zend_update_property(FireBird_Db_Info_ce, Z_OBJ_P(Db_Info), \
            #item, sizeof(#item) - 1, &str_arr);                    \
        zval_ptr_dtor(&str_arr);                                    \
    } break

    FBDEBUG("Parsing DB info buffer");

    for (IXpbBuilder_rewind(dpb, st); !IXpbBuilder_isEof(dpb, st); IXpbBuilder_moveNext(dpb, st)) {
        unsigned char tag = IXpbBuilder_getTag(dpb, st);
        const ISC_UCHAR* b = IXpbBuilder_getBytes(dpb, st);
        len = IXpbBuilder_getLength(dpb, st);

        switch(tag) {
            READ_STR_ARRAY(isc_version);
            READ_STR_ARRAY(firebird_version);

            // TODO: format date/time
            case isc_info_creation_date: {
                struct tm t;
                isc_decode_timestamp((ISC_TIMESTAMP *) b, &t);
                zend_update_property_long(FireBird_Db_Info_ce, Z_OBJ_P(Db_Info), "creation_date", sizeof("creation_date") - 1, mktime(&t));
            } break;

            READ_BIGINT(reads);
            READ_BIGINT(writes);
            READ_BIGINT(fetches);
            READ_BIGINT(marks);
            READ_BIGINT(page_size);
            READ_BIGINT(num_buffers);
            case isc_info_limbo: {
                if(limbo_count++ < max_limbo_count) {
                    add_next_index_long(&info_limbo, IXpbBuilder_getBigInt(dpb, st));
                } else {
                    IXpbBuilder_moveNext(dpb, st);
                }
            } break;
            READ_BIGINT(current_memory);
            READ_BIGINT(max_memory);

            READ_BIGINT(allocation);
            READ_BIGINT(attachment_id);
            READ_BIGINT(read_seq_count);
            READ_BIGINT(read_idx_count);
            READ_BIGINT(insert_count);
            READ_BIGINT(update_count);
            READ_BIGINT(delete_count);
            READ_BIGINT(backout_count);
            READ_BIGINT(purge_count);
            READ_BIGINT(expunge_count);

            READ_BIGINT(sweep_interval);
            READ_BIGINT(ods_version);
            READ_BIGINT(ods_minor_version);
            READ_BIGINT(no_reserve);
            READ_BIGINT(forced_writes);

            READ_BIGINT(set_page_buffers);
            READ_BIGINT(db_sql_dialect);
            READ_BIGINT(db_read_only);
            READ_BIGINT(db_size_in_pages);

            READ_BIGINT(oldest_transaction);
            READ_BIGINT(oldest_active);
            READ_BIGINT(oldest_snapshot);
            READ_BIGINT(next_transaction);
            READ_BIGINT(db_file_size); // always 0 for me
            READ_BIGINT_FB(pages_used);
            READ_BIGINT_FB(pages_free);
            READ_BIGINT_FB(ses_idle_timeout_db);
            READ_BIGINT_FB(ses_idle_timeout_att);
            READ_BIGINT_FB(ses_idle_timeout_run);
            READ_BIGINT_FB(conn_flags);
            READ_BIGINT_FB(crypt_state);
            READ_BIGINT_FB(statement_timeout_db);
            READ_BIGINT_FB(statement_timeout_att);
            READ_BIGINT_FB(protocol_version);
            READ_STR_FB(crypt_key);
            READ_STR_FB(crypt_plugin);
            READ_STR_FB(wire_crypt);

            READ_BIGINT_FB(next_attachment);
            READ_BIGINT_FB(next_statement);
            READ_BIGINT_FB(db_guid);
            READ_BIGINT_FB(db_file_id);
            READ_BIGINT_FB(replica_mode);
            READ_STR_FB(username);
            READ_STR_FB(sqlrole);
            READ_BIGINT_FB(parallel_workers);

            // All fields populated, not sure if needed
            // case fb_info_features: {
            //     for (unsigned i = 0; i < len; i++){
            //         if (b[i] == 0) {
            //             _php_firebird_module_error("Bad provider feature value: %d", b[i]);
            //         } else if (b[i] < fb_feature_max) {
            //             FBDEBUG_NOFL("  feature: %d", b[i]);
            //         }
            //     }
            // } break;

            case isc_info_end: break;
            case isc_info_truncated: {
                fbp_error("DB info buffer error: truncated");
            } return FAILURE;
            case isc_info_error: {
                fbp_error("DB info buffer error");
            } return FAILURE;

            default: {
                dump_buffer(b, len);
                fbp_fatal("BUG! Unhandled DB info tag: %d", tag);
            } break;
        }
    }

    zend_update_property(FireBird_Db_Info_ce, Z_OBJ_P(Db_Info), "limbo", sizeof("limbo") - 1, &info_limbo);

    return SUCCESS;
}
