// #include <firebird/fb_c_api.h>

#include "php.h"
#include "php_firebird.h"
#include "zend_exceptions.h"

#include "service.h"
#include "fbp_service.h"

zend_class_entry *FireBird_Service_ce;
zend_class_entry *FireBird_Server_Info_ce;
zend_class_entry *FireBird_Server_Db_Info_ce;
zend_class_entry *FireBird_Server_User_Info_ce;
zend_class_entry *FireBird_Service_Connect_Args_ce;

static zend_object_handlers FireBird_Service_object_handlers;

int FireBird_Service_connect(zval *Service, zval *Service_Connect_Args)
{
    firebird_service *svc = get_firebird_service_from_zval(Service);

    if (fbp_service_connect(svc, Service_Connect_Args)) {
        update_err_props(FBG(status), FireBird_Service_ce, Service);
        return FAILURE;
    }

    PROP_SET(FireBird_Service_ce, Service, "args", Service_Connect_Args);

    return SUCCESS;
}

PHP_METHOD(FireBird_Service, connect)
{
    zval *Service_Connect_Args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Service_Connect_Args, FireBird_Service_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    RETVAL_BOOL(SUCCESS == FireBird_Service_connect(ZEND_THIS, Service_Connect_Args));
}

PHP_METHOD(FireBird_Service, disconnect) {
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    FBDEBUG("Service::disconnect()");

    if(svc->svc_handle) {
        FBDEBUG_NOFL("  Closing handle: %d", svc->svc_handle);
        if(isc_service_detach(FBG(status), &svc->svc_handle)){
            update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        } else {
            svc->svc_handle = 0;
            RETURN_TRUE;
        }
    }

    RETURN_FALSE;
}

PHP_METHOD(FireBird_Service, get_server_info)
{
    ZEND_PARSE_PARAMETERS_NONE();

    char info_resp[1024] = { 0 };

    // TODO: separate only SYSDBA accessible information
    static char info_req[] = {
        isc_info_svc_server_version,
        isc_info_svc_implementation,
        isc_info_svc_get_env,
        isc_info_svc_get_env_lock,
        isc_info_svc_get_env_msg,
        isc_info_svc_user_dbpath,
        isc_info_svc_svr_db_info,
        isc_info_svc_get_users,
    };

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    object_init_ex(return_value, FireBird_Server_Info_ce);
    if(fbp_service_get_server_info(svc, return_value, sizeof(info_req), info_req, sizeof(info_resp), info_resp)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

PHP_METHOD(FireBird_Service, add_user)
{
    zval *User_Info = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(User_Info, FireBird_Server_User_Info_ce)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (FAILURE == fbp_service_addmod_user(svc, User_Info, isc_action_svc_add_user)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, modify_user)
{
    zval *User_Info = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(User_Info, FireBird_Server_User_Info_ce)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (FAILURE == fbp_service_addmod_user(svc, User_Info, isc_action_svc_modify_user)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, delete_user)
{
    char *username;
    size_t username_len;

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(username, username_len)
    ZEND_PARSE_PARAMETERS_END();

    if (fbp_service_delete_user(svc, username, username_len)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, backup)
{
    char *dbname, *bkpname;
    zend_long dbname_len, bkpname_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(dbname, dbname_len)
        Z_PARAM_STRING(bkpname, bkpname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_backup(svc, dbname_len, dbname, bkpname_len, bkpname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, restore)
{
    char *dbname, *bkpname;
    zend_long dbname_len, bkpname_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(bkpname, bkpname_len)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_restore(svc, bkpname_len, bkpname, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, shutdown_db)
{
    char *dbname;
    size_t dbname_len;
    zend_long mode = 0, timeout = 0;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STRING(dbname, dbname_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(mode)
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_shutdown_db(svc, dbname_len, dbname, mode, timeout)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, db_online)
{
    char *dbname;
    size_t dbname_len;
    zend_long mode = 0, timeout = 0;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STRING(dbname, dbname_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(mode)
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_db_online(svc, dbname_len, dbname, mode)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, set_page_buffers)
{
    char *dbname;
    size_t dbname_len;
    zend_long buffers;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(dbname, dbname_len)
        Z_PARAM_LONG(buffers)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_set_page_buffers(svc, dbname_len, dbname, buffers)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, set_sweep_interval)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(dbname, dbname_len)
        Z_PARAM_LONG(interval)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_set_sweep_interval(svc, dbname_len, dbname, interval)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, deny_new_attachments)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_deny_new_attachments(svc, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, deny_new_transactions)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_deny_new_transactions(svc, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, set_write_mode_async)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_set_write_mode_async(svc, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, set_write_mode_sync)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_set_write_mode_sync(svc, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, set_access_mode_readonly)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_set_access_mode_readonly(svc, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, set_access_mode_readwrite)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_set_access_mode_readwrite(svc, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, enable_reserve_space)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_enable_reserve_space(svc, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, disable_reserve_space)
{
    char *dbname;
    size_t dbname_len;
    zend_long interval;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dbname, dbname_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_disable_reserve_space(svc, dbname_len, dbname)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Service, set_sql_dialect)
{
    char *dbname;
    size_t dbname_len;
    zend_long dialect;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(dbname, dbname_len)
        Z_PARAM_LONG(dialect)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    if (fbp_service_set_sql_dialect(svc, dbname_len, dbname, dialect)) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

// const zend_function_entry FireBird_Service_methods[] = {
//     PHP_ME(Service, connect, arginfo_FireBird_Service_connect, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, disconnect, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, get_server_info, arginfo_FireBird_Service_get_server_info, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, add_user, arginfo_FireBird_Service_add_user, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, modify_user, arginfo_FireBird_Service_add_user, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, delete_user, arginfo_FireBird_Service_delete_user, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, backup, arginfo_FireBird_Service_backup, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, restore, arginfo_FireBird_Service_restore, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, shutdown_db, arginfo_FireBird_Service_shutdown_db, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, db_online, arginfo_FireBird_Service_db_online, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, set_page_buffers, arginfo_FireBird_Service_set_page_buffers, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, set_sweep_interval, arginfo_FireBird_Service_set_sweep_interval, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, deny_new_attachments, arginfo_FireBird_Service_dbname_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, deny_new_transactions, arginfo_FireBird_Service_dbname_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, set_write_mode_async, arginfo_FireBird_Service_dbname_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, set_write_mode_sync, arginfo_FireBird_Service_dbname_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, set_access_mode_readonly, arginfo_FireBird_Service_dbname_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, set_access_mode_readwrite, arginfo_FireBird_Service_dbname_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, enable_reserve_space, arginfo_FireBird_Service_dbname_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, disable_reserve_space, arginfo_FireBird_Service_dbname_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Service, set_sql_dialect, arginfo_FireBird_Service_set_sql_dialect, ZEND_ACC_PUBLIC)

//     PHP_FE_END
// };

static zend_object *FireBird_Service_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Service_create()");

    firebird_service *s = zend_object_alloc(sizeof(firebird_service), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Service_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Service_free_obj");

    firebird_service *svc = get_firebird_service_from_obj(obj);

    if(svc->svc_handle) {
        if(isc_service_detach(FBG(status), &svc->svc_handle)) {
            fbp_status_error(FBG(status));
        } else {
            svc->svc_handle = 0;
        }
    }

    zend_object_std_dtor(&svc->std);
}

// void register_FireBird_Service_ce()
// {
//     zend_class_entry tmp_ce;
//     INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Service", FireBird_Service_methods);
//     FireBird_Service_ce = zend_register_internal_class(&tmp_ce);

//     DECLARE_PROP_OBJ(FireBird_Service_ce, args, FireBird\\Service_Connect_Args, ZEND_ACC_PROTECTED_SET);
//     DECLARE_ERR_PROPS(FireBird_Service_ce);

//     zend_class_implements(FireBird_Service_ce, 1, FireBird_IError_ce);

//     FireBird_Service_ce->create_object = FireBird_Service_create;
//     FireBird_Service_ce->default_object_handlers = &FireBird_Service_object_handlers;

//     memcpy(&FireBird_Service_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

//     FireBird_Service_object_handlers.offset = XtOffsetOf(firebird_service, std);
//     FireBird_Service_object_handlers.free_obj = FireBird_Service_free_obj;
// }

void register_FireBird_Server_Info_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Server_Info", NULL);
    FireBird_Server_Info_ce = zend_register_internal_class(&tmp_ce);

    fbp_declare_props_from_zmap(FireBird_Server_Info_ce, &fbp_server_info_zmap);

    DECLARE_PROP_OBJ(FireBird_Server_Info_ce, db_info, FireBird\\Server_Db_Info, ZEND_ACC_PUBLIC);
    DECLARE_PROP_ARRAY(FireBird_Server_Info_ce, users, ZEND_ACC_PUBLIC);
}

void register_FireBird_Server_Db_Info_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Server_Db_Info", NULL);
    FireBird_Server_Db_Info_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_LONG(FireBird_Server_Db_Info_ce, num_att, ZEND_ACC_PUBLIC);
    DECLARE_PROP_LONG(FireBird_Server_Db_Info_ce, num_db, ZEND_ACC_PUBLIC);
    DECLARE_PROP_ARRAY(FireBird_Server_Db_Info_ce, dbname, ZEND_ACC_PUBLIC);
}

void register_FireBird_Server_User_Info_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Server_User_Info", NULL);
    FireBird_Server_User_Info_ce = zend_register_internal_class(&tmp_ce);

    fbp_declare_props_from_zmap(FireBird_Server_User_Info_ce, &fbp_user_info_zmap);
}

void register_FireBird_Service_Connect_Args_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Service_Connect_Args", NULL);
    FireBird_Service_Connect_Args_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_STRING(FireBird_Service_Connect_Args_ce, service_name, ZEND_ACC_PUBLIC);
    fbp_declare_props_from_zmap(FireBird_Service_Connect_Args_ce, &fbp_service_connect_zmap);
}
