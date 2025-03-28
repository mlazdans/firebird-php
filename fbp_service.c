#include <firebird/fb_c_api.h>
#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"

#include "service.h"
#include "fbp_service.h"

firebird_xpb_zmap fbp_service_connect_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_spb_user_name, isc_spb_password
    }),
    ((const char *[]){
        "user_name", "password"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING
    })
);

firebird_xpb_zmap fbp_server_info_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_info_svc_server_version, isc_info_svc_implementation, isc_info_svc_get_env, isc_info_svc_get_env_lock, isc_info_svc_get_env_msg,
        isc_info_svc_user_dbpath
    }),
    ((const char *[]){
        "server_version", "implementation", "get_env", "get_env_lock", "get_env_msg",
        "user_dbpath"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING,
        MAY_BE_STRING
    })
);

firebird_xpb_zmap fbp_user_info_zmap = XPB_ZMAP_INIT(
    ((const char []){
        isc_spb_sec_username, isc_spb_sec_password, isc_spb_sec_firstname, isc_spb_sec_middlename,
        isc_spb_sec_lastname, isc_spb_sec_admin
    }),
    ((const char *[]){
        "username", "password", "firstname", "middlename",
        "lastname", "admin"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING,
        MAY_BE_STRING, MAY_BE_TRUE | MAY_BE_FALSE
    })
);

fbp_object_accessor(firebird_service);

int fbp_service_build_dpb(zend_class_entry *ce, zval *Args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written)
{
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_SPB_ATTACH, NULL, 0);

    xpb_insert_tag(isc_spb_version3);
    fbp_insert_xpb_from_zmap(ce, Args, xpb_zmap, xpb, st);

    *num_dpb_written = IXpbBuilder_getBufferLength(xpb, st);
    *dpb_buf = IXpbBuilder_getBuffer(xpb, st);

    return SUCCESS;
}

int fbp_service_connect(firebird_service *svc, zval *Service_Connect_Args)
{
    zval rv, *service_name;
    const char *dpb_buffer;
    short num_dpb_written;

    service_name = OBJ_GET(FireBird_Service_Connect_Args_ce, Service_Connect_Args, "service_name", &rv);

    if (fbp_service_build_dpb(FireBird_Service_Connect_Args_ce, Service_Connect_Args,
        &fbp_service_connect_zmap, &dpb_buffer, &num_dpb_written)) {
            return FAILURE;
    }

    FBDEBUG("Service::connect: %s", Z_STRVAL_P(service_name));
    if (isc_service_attach(FBG(status), (short)Z_STRLEN_P(service_name), Z_STRVAL_P(service_name),
        &svc->svc_handle, num_dpb_written, dpb_buffer)) {
            return FAILURE;
    }
    FBDEBUG_NOFL("  connected, handle: %d", svc->svc_handle);

    return SUCCESS;
}

int fbp_service_get_server_info(firebird_service *svc, zval *Server_Info,
    size_t req_size, char *req_buff,
    size_t resp_size, char *resp_buff)
{

    static char spb[] = { isc_info_svc_timeout, 10, 0, 0, 0 };
    static char action[] = { isc_action_svc_display_user_adm };

    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, sizeof(action), action)) {
        return FAILURE;
    }

    if (isc_service_query(FBG(status), &svc->svc_handle, NULL, sizeof(spb), spb,
        req_size, req_buff, resp_size, resp_buff)) {
            return FAILURE;
    }

    FBDEBUG("Parsing server info buffer");

    zval users;
    array_init(&users);

    ISC_INT64 len, total_len = 0;
    unsigned char tag, get_users_requested = 0;
    char* p = resp_buff;
    const char* const end = p + resp_size;

#define READ_SI_STRING(name)                               \
    case isc_info_svc_##name: {                            \
        len = isc_portable_integer(p, 2);                  \
        p += 2;                                            \
        zend_update_property_stringl(                      \
            FireBird_Server_Info_ce, Z_OBJ_P(Server_Info), \
            #name, sizeof(#name) - 1, p, len);             \
        p += len;                                          \
    } break

#define READ_UI_STRING(name)                                       \
    case isc_spb_sec_##name: {                                     \
        len = isc_portable_integer(p, 2); p += 2;                  \
        zend_update_property_stringl(FireBird_Server_User_Info_ce, \
            Z_OBJ(user_info), #name, sizeof(#name) - 1, p, len);   \
    } break

#define READ_UI_LONG(name)                                      \
    case isc_spb_sec_##name: {                                  \
        len = 4;                                                \
        zend_update_property_long(FireBird_Server_User_Info_ce, \
            Z_OBJ(user_info), #name, sizeof(#name) - 1,         \
            isc_portable_integer(p, len));                      \
    } break

    while (p < end && *p != isc_info_end) {
        tag = *p++;
        FBDEBUG_NOFL("  tag: %d", tag);

        switch(tag) {
            READ_SI_STRING(server_version);
            READ_SI_STRING(implementation);
            READ_SI_STRING(get_env);
            READ_SI_STRING(get_env_lock);
            READ_SI_STRING(get_env_msg);
            READ_SI_STRING(user_dbpath);

            case isc_info_svc_svr_db_info: {
                FBDEBUG("Parsing db info block");
                zval server_db_info, dbname;
                object_init_ex(&server_db_info, FireBird_Server_Db_Info_ce);
                array_init(&dbname);
                while (p < end && *p != isc_info_flag_end && *p != isc_info_end) {
                    switch (*p++) {
                        case isc_spb_num_att: {
                            zend_update_property_long(FireBird_Server_Db_Info_ce, Z_OBJ(server_db_info), "num_att", sizeof("num_att") - 1, isc_portable_integer(p, 4));
                            len = 4;
                        } break;

                        case isc_spb_num_db: {
                            zend_update_property_long(FireBird_Server_Db_Info_ce, Z_OBJ(server_db_info), "num_db", sizeof("num_db") - 1, isc_portable_integer(p, 4));
                            len = 4;
                        } break;

                        case isc_spb_dbname: {
                            len = isc_portable_integer(p, 2); p += 2;
                            add_next_index_stringl(&dbname, p, len);
                        } break;

                        default: {
                            fbp_fatal("BUG! Unhandled isc_info_svc_svr_db_info tag: %d", (char)*(p - 1));
                        } break;
                    }
                    p += len;
                }
                p++;
                zend_update_property(FireBird_Server_Db_Info_ce, Z_OBJ(server_db_info), "dbname", sizeof("dbname") - 1, &dbname);
                zend_update_property(FireBird_Server_Info_ce, Z_OBJ_P(Server_Info), "db_info", sizeof("db_info") - 1, &server_db_info);
                zval_ptr_dtor(&server_db_info);
                zval_ptr_dtor(&dbname);
            } break;

            case isc_info_svc_get_users: {
                get_users_requested = 1;
                FBDEBUG("Parsing user info block: %d bytes", isc_portable_integer(p, 2));
                p += 2;

                zval user_info;

                while (p < end && *p != isc_info_end) {
                    if (*p == isc_spb_sec_username) { // Assuming isc_spb_sec_username always first?
                        if (Z_TYPE(user_info) == IS_OBJECT) {
                            add_next_index_zval(&users, &user_info);
                        }
                        object_init_ex(&user_info, FireBird_Server_User_Info_ce);
                    }

                    switch (*p++) {
                        READ_UI_STRING(username);
                        READ_UI_STRING(firstname);
                        READ_UI_STRING(middlename);
                        READ_UI_STRING(lastname);
                        READ_UI_LONG(admin);

                        // Skip legacy stuff
                        case isc_spb_sec_userid:
                        case isc_spb_sec_groupid: {
                            len = 4;
                        } break;
                        case isc_spb_sec_groupname: {
                            len = isc_portable_integer(p, 2); p += 2;
                        } break;

                        case isc_info_truncated: {
                            fbp_warning("Server user info buffer error: truncated");
                        } goto error;

                        case isc_info_error: {
                            fbp_warning("Server user info buffer error");
                        } goto error;

                        default: {
                            fbp_fatal("BUG! Unhandled isc_info_svc_get_users tag: %d", (char)*(p - 1));
                        } break;
                    }
                    p += len;
                }
                if (Z_TYPE(user_info) == IS_OBJECT) {
                    add_next_index_zval(&users, &user_info);
                }
            } break;

            case isc_info_truncated: {
                fbp_warning("Server info buffer error: truncated");
            } goto error;
            case isc_info_error: {
                fbp_warning("Server info buffer error");
            } goto error;

            default: {
                fbp_fatal("BUG! Unhandled isc_info_svc_* tag: %d", tag);
            } break;
        }
    }

    if (get_users_requested) {
        zend_update_property(FireBird_Server_Info_ce, Z_OBJ_P(Server_Info), "users", sizeof("users") - 1, &users);
    }
    zval_ptr_dtor(&users);

    return SUCCESS;

error:
    zval_ptr_dtor(&users);
    return FAILURE;
}

int fbp_service_addmod_user(firebird_service *svc, zval *User_Info, const ISC_UCHAR tag)
{
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_SPB_START, NULL, 0);

    IXpbBuilder_insertTag(xpb, st, tag);

    fbp_insert_xpb_from_zmap(FireBird_Server_User_Info_ce, User_Info, &fbp_user_info_zmap, xpb, st);

    fbp_dump_buffer(IXpbBuilder_getBufferLength(xpb, st), IXpbBuilder_getBuffer(xpb, st));

    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, IXpbBuilder_getBufferLength(xpb, st), IXpbBuilder_getBuffer(xpb, st))) {
        return FAILURE;
    }

    return SUCCESS;
}

int fbp_service_delete_user(firebird_service *svc, const char *username, ISC_USHORT username_len)
{
    char buf[128] = {0};
    char *p = buf;

    *p++ = isc_action_svc_delete_user;
    *p++ = isc_spb_sec_username;

    ISC_UCHAR bytes_num = min(username_len, sizeof(buf) - (p - buf) - 2);

    fbp_store_portable_integer(p, bytes_num, 2); p += 2;
    memcpy(p, username, bytes_num); p += bytes_num;

    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, p - buf, buf)) {
        return FAILURE;
    }

    return SUCCESS;
}

#define spb_insert_db(dbname_len, dbname) spb_insert_str(isc_spb_dbname, dbname_len, dbname)
#define spb_insert_str(tag, str_len, str) do {                             \
        *p++ = tag;                                                        \
        ISC_UCHAR bytes_count = min(str_len, sizeof(buf) - (p - buf) - 2); \
        fbp_store_portable_integer(p, bytes_count, 2); p += 2;             \
        memcpy(p, str, bytes_count); p += bytes_count;                     \
    } while(0)

int fbp_service_shutdown_db(firebird_service *svc, size_t dbname_len, char *dbname, firebird_shutdown_mode mode, ISC_ULONG timeout)
{
    char buf[256] = {0};
    char *p = buf;

    *p++ = isc_action_svc_properties;
    spb_insert_db(dbname_len, dbname);
    *p++ = isc_spb_prp_shutdown_db;
    fbp_store_portable_integer(p, timeout * 10, 4); p += 4;
    *p++ = isc_spb_prp_shutdown_mode;
    *p++ = mode;

    fbp_dump_buffer(p - buf, buf);
    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, (ISC_USHORT)(p - buf), buf)) {
        return FAILURE;
    }

    return SUCCESS;
}

int fbp_service_db_online(firebird_service *svc, size_t dbname_len, char *dbname, firebird_shutdown_mode mode)
{
    char buf[256] = {0};
    char *p = buf;

    *p++ = isc_action_svc_properties;
    spb_insert_db(dbname_len, dbname);
    *p++ = isc_spb_options;
    fbp_store_portable_integer(p, isc_spb_prp_db_online, 4); p += 4;
    *p++ = isc_spb_prp_online_mode;
    *p++ = mode;

    fbp_dump_buffer(p - buf, buf);
    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, (ISC_USHORT)(p - buf), buf)) {
        return FAILURE;
    }

    return SUCCESS;
}

int _fbp_service_db_maint_with_arg(firebird_service *svc, size_t dbname_len, char *dbname, ISC_UCHAR command, ISC_ULONG arg)
{
    char buf[256] = {0};
    char *p = buf;

    *p++ = isc_action_svc_properties;
    spb_insert_db(dbname_len, dbname);
    *p++ = command;
    fbp_store_portable_integer(p, arg, 4); p += 4;

    fbp_dump_buffer(p - buf, buf);
    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, (ISC_USHORT)(p - buf), buf)) {
        return FAILURE;
    }

    return SUCCESS;
}

int _fbp_service_db_maint_with_tag(firebird_service *svc, size_t dbname_len, char *dbname, ISC_UCHAR command, ISC_UCHAR tag)
{
    char buf[256] = {0};
    char *p = buf;

    *p++ = isc_action_svc_properties;
    spb_insert_db(dbname_len, dbname);
    *p++ = command;
    *p++ = tag;

    fbp_dump_buffer(p - buf, buf);
    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, (ISC_USHORT)(p - buf), buf)) {
        return FAILURE;
    }

    return SUCCESS;
}

int fbp_service_set_page_buffers(firebird_service *svc, size_t dbname_len, char *dbname, ISC_ULONG buffers)
{
    return _fbp_service_db_maint_with_arg(svc, dbname_len, dbname, isc_spb_prp_page_buffers, buffers);
}

int fbp_service_set_sweep_interval(firebird_service *svc, size_t dbname_len, char *dbname, ISC_ULONG interval)
{
    return _fbp_service_db_maint_with_arg(svc, dbname_len, dbname, isc_spb_prp_sweep_interval, interval);
}

int fbp_service_deny_new_attachments(firebird_service *svc, size_t dbname_len, char *dbname)
{
    return _fbp_service_db_maint_with_arg(svc, dbname_len, dbname, isc_spb_prp_deny_new_attachments, 0);
}

int fbp_service_deny_new_transactions(firebird_service *svc, size_t dbname_len, char *dbname)
{
    return _fbp_service_db_maint_with_arg(svc, dbname_len, dbname, isc_spb_prp_deny_new_transactions, 0);
}

int fbp_service_set_write_mode_async(firebird_service *svc, size_t dbname_len, char *dbname)
{
    return _fbp_service_db_maint_with_tag(svc, dbname_len, dbname, isc_spb_prp_write_mode, isc_spb_prp_wm_async);
}

int fbp_service_set_write_mode_sync(firebird_service *svc, size_t dbname_len, char *dbname)
{
    return _fbp_service_db_maint_with_tag(svc, dbname_len, dbname, isc_spb_prp_write_mode, isc_spb_prp_wm_sync);
}

int fbp_service_set_access_mode_readonly(firebird_service *svc, size_t dbname_len, char *dbname)
{
    return _fbp_service_db_maint_with_tag(svc, dbname_len, dbname, isc_spb_prp_access_mode, isc_spb_prp_am_readonly);
}

int fbp_service_set_access_mode_readwrite(firebird_service *svc, size_t dbname_len, char *dbname)
{
    return _fbp_service_db_maint_with_tag(svc, dbname_len, dbname, isc_spb_prp_access_mode, isc_spb_prp_am_readwrite);
}

int fbp_service_enable_reserve_space(firebird_service *svc, size_t dbname_len, char *dbname)
{
    return _fbp_service_db_maint_with_tag(svc, dbname_len, dbname, isc_spb_prp_reserve_space, isc_spb_prp_res);
}

int fbp_service_disable_reserve_space(firebird_service *svc, size_t dbname_len, char *dbname)
{
    return _fbp_service_db_maint_with_tag(svc, dbname_len, dbname, isc_spb_prp_reserve_space, isc_spb_prp_res_use_full);
}

int fbp_service_set_sql_dialect(firebird_service *svc, size_t dbname_len, char *dbname, ISC_ULONG dialect)
{
    return _fbp_service_db_maint_with_arg(svc, dbname_len, dbname, isc_spb_prp_set_sql_dialect, dialect);
}

// Backup / restore service is just a wrapper around gbak utility.
// Some args you can pass as flags/tags:
//   isc_spb_res_create | isc_spb_res_replace via isc_spb_options for ibase_restore()
//   isc_spb_verbose for both backup/restore as a tag
//   isc_spb_bkp_length, isc_spb_res_length as 32-bit
// Most other seems strings or buffers of some kind.
// Response is streamed line by line as would gbak.
// Not sure I want implement this today.
// #define isc_spb_bkp_file                 5
// #define isc_spb_bkp_factor               6
// #define isc_spb_bkp_length               7
// #define isc_spb_bkp_skip_data            string
// #define isc_spb_bkp_stat                 15
// #define isc_spb_bkp_keyholder			 16
// #define isc_spb_bkp_keyname				 17
// #define isc_spb_bkp_crypt				 18
// #define isc_spb_bkp_include_data         string
// #define isc_spb_bkp_parallel_workers	 21
// #define isc_spb_bkp_ignore_checksums     0x01
// #define isc_spb_bkp_ignore_limbo         0x02
// #define isc_spb_bkp_metadata_only        0x04
// #define isc_spb_bkp_no_garbage_collect   0x08
// #define isc_spb_bkp_old_descriptions     0x10
// #define isc_spb_bkp_non_transportable    0x20
// #define isc_spb_bkp_convert              0x40
// #define isc_spb_bkp_expand				 0x80
// #define isc_spb_bkp_no_triggers			 0x8000
// #define isc_spb_bkp_zip					 0x010000
// #define isc_spb_bkp_direct_io			 0x020000
int fbp_service_backup(firebird_service *svc, size_t dbname_len, char *dbname, size_t bkpname_len, char *bkpname)
{
    char buf[256] = {0};
    char *p = buf;

    *p++ = isc_action_svc_backup;
    spb_insert_str(isc_spb_dbname, dbname_len, dbname);
    spb_insert_str(isc_spb_bkp_file, bkpname_len, bkpname);

    fbp_dump_buffer(p - buf, buf);

    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, (ISC_USHORT)(p - buf), buf)) {
        return FAILURE;
    }

    return SUCCESS;
}

int fbp_service_restore(firebird_service *svc, size_t bkpname_len, char *bkpname, size_t dbname_len, char *dbname)
{
    char buf[256] = {0};
    char *p = buf;

    *p++ = isc_action_svc_restore;
    spb_insert_str(isc_spb_dbname, dbname_len, dbname);
    spb_insert_str(isc_spb_bkp_file, bkpname_len, bkpname);

    fbp_dump_buffer(p - buf, buf);

    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, (ISC_USHORT)(p - buf), buf)) {
        return FAILURE;
    }

    return SUCCESS;
}
