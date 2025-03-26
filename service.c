#include <firebird/fb_c_api.h>

#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"
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

    OBJ_SET(FireBird_Service_ce, Service, "args", Service_Connect_Args);

    return SUCCESS;
}

PHP_METHOD(Service, connect)
{
    zval *Service_Connect_Args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Service_Connect_Args, FireBird_Service_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    RETVAL_BOOL(SUCCESS == FireBird_Service_connect(ZEND_THIS, Service_Connect_Args));
}

PHP_METHOD(Service, disconnect) {
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

PHP_METHOD(Service, get_server_info)
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

PHP_METHOD(Service, add_user)
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

PHP_METHOD(Service, modify_user)
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

PHP_METHOD(Service, delete_user)
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

// TODO: class with parameters for both backup and restore instead of options flags?
// TODO: no options for both backup and restore yet
// For isc_spb_options
// firebird\src\utilities\fbsvcmgr\fbsvcmgr.cpp
// backupOptions, restoreOptions
PHP_METHOD(Service, backup)
{
    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);
    zend_string *dbname, *bkp_file;
    zend_long options = 0;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(dbname)
        Z_PARAM_STR(bkp_file)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(options)
    ZEND_PARSE_PARAMETERS_END();

    FBDEBUG("Service::backup()");
    FBDEBUG_NOFL("  dbname: %s", ZSTR_VAL(dbname));
    FBDEBUG_NOFL("  bkp_file: %s", ZSTR_VAL(bkp_file));
    FBDEBUG_NOFL("  options: %d", options);

    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_SPB_START, NULL, 0);

    IXpbBuilder_insertTag(xpb, st, isc_action_svc_backup);
    IXpbBuilder_insertString(xpb, st, isc_spb_dbname, ZSTR_VAL(dbname));
    IXpbBuilder_insertString(xpb, st, isc_spb_bkp_file, ZSTR_VAL(bkp_file));

    fbp_dump_buffer(IXpbBuilder_getBufferLength(xpb, st), IXpbBuilder_getBuffer(xpb, st));

    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, IXpbBuilder_getBufferLength(xpb, st), IXpbBuilder_getBuffer(xpb, st))) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

// TODO: code dup with backup
PHP_METHOD(Service, restore)
{
    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);
    zend_string *dbname, *bkp_file;
    zend_long options = 0;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(bkp_file)
        Z_PARAM_STR(dbname)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(options)
    ZEND_PARSE_PARAMETERS_END();

    FBDEBUG("Service::backup()");
    FBDEBUG_NOFL("  dbname: %s", ZSTR_VAL(dbname));
    FBDEBUG_NOFL("  bkp_file: %s", ZSTR_VAL(bkp_file));
    FBDEBUG_NOFL("  options: %d", options);

    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_SPB_START, NULL, 0);

    IXpbBuilder_insertTag(xpb, st, isc_action_svc_restore);
    IXpbBuilder_insertString(xpb, st, isc_spb_bkp_file, ZSTR_VAL(bkp_file));
    IXpbBuilder_insertString(xpb, st, isc_spb_dbname, ZSTR_VAL(dbname));

    fbp_dump_buffer(IXpbBuilder_getBufferLength(xpb, st), IXpbBuilder_getBuffer(xpb, st));

    if (isc_service_start(FBG(status), &svc->svc_handle, NULL, IXpbBuilder_getBufferLength(xpb, st), IXpbBuilder_getBuffer(xpb, st))) {
        update_err_props(FBG(status), FireBird_Service_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

const zend_function_entry FireBird_Service_methods[] = {
    PHP_ME(Service, connect, arginfo_FireBird_Service_connect, ZEND_ACC_PUBLIC)
    PHP_ME(Service, disconnect, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Service, get_server_info, arginfo_FireBird_Service_get_server_info, ZEND_ACC_PUBLIC)
    PHP_ME(Service, add_user, arginfo_FireBird_Service_add_user, ZEND_ACC_PUBLIC)
    PHP_ME(Service, modify_user, arginfo_FireBird_Service_add_user, ZEND_ACC_PUBLIC)
    PHP_ME(Service, delete_user, arginfo_FireBird_Service_delete_user, ZEND_ACC_PUBLIC)
    PHP_ME(Service, backup, arginfo_FireBird_Service_backup, ZEND_ACC_PUBLIC)
    PHP_ME(Service, restore, arginfo_FireBird_Service_restore, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

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
            status_fbp_error(FBG(status));
        } else {
            svc->svc_handle = 0;
        }
    }

    zend_object_std_dtor(&svc->std);
}

void register_FireBird_Service_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Service", FireBird_Service_methods);
    FireBird_Service_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Service_ce, args, FireBird\\Service_Connect_Args, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Service_ce);

    zend_class_implements(FireBird_Service_ce, 1, FireBird_IError_ce);

    FireBird_Service_ce->create_object = FireBird_Service_create;
    FireBird_Service_ce->default_object_handlers = &FireBird_Service_object_handlers;

    memcpy(&FireBird_Service_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Service_object_handlers.offset = XtOffsetOf(firebird_service, std);
    FireBird_Service_object_handlers.free_obj = FireBird_Service_free_obj;
}

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
