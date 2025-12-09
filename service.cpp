#include "fbp/service.hpp"
#include "firebird_php.hpp"
#include "firebird_utils.h"

extern "C" {

#include "php.h"

fbp_object_accessor(firebird_service);
static zend_object_handlers FireBird_Service_object_handlers;

firebird_xpb_zmap fbp_service_connect_zmap = XPB_ZMAP_INIT(
    ((const uint8_t[]){
        isc_spb_user_name, isc_spb_password
    }),
    ((const char *[]){
        "user_name", "password"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING
    })
);

firebird_xpb_zmap fbp_user_info_zmap = XPB_ZMAP_INIT(
    ((const uint8_t[]){
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

PHP_METHOD(FireBird_Service, __construct)
{
}

PHP_METHOD(FireBird_Service, connect)
{
    zval *args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(args, FireBird_Service_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Service_ce);
    firebird_service *svc = get_firebird_service_from_zval(return_value);

    if (fbu_service_init(&svc->svh)) {
        RETURN_THROWS();
    }

    if (fbu_service_connect(svc->svh, args)) {
        RETURN_THROWS();
    }

    PROP_SET(FireBird_Service_ce, return_value, "args", args);
}

PHP_METHOD(FireBird_Service, disconnect)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);
    if (fbu_service_disconnect(svc->svh)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Service, get_server_info)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    object_init_ex(return_value, FireBird_Server_Info_ce);
    if (fbu_service_get_server_info(svc->svh, return_value)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Service, get_db_info)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    object_init_ex(return_value, FireBird_Server_Db_Info_ce);
    if (fbu_service_get_db_info(svc->svh, return_value)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Service, get_users)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    array_init(return_value);
    if (fbu_service_get_users(svc->svh, return_value)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Service, add_user)
{
    zval *ui = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(ui, FireBird_User_Info_ce)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);
    if (fbu_service_add_user(svc->svh, ui)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Service, modify_user)
{
    zval *ui = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(ui, FireBird_User_Info_ce)
    ZEND_PARSE_PARAMETERS_END();

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);
    if (fbu_service_modify_user(svc->svh, ui)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Service, delete_user)
{
    char *username;
    size_t username_len;

    firebird_service *svc = get_firebird_service_from_zval(ZEND_THIS);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(username, username_len)
    ZEND_PARSE_PARAMETERS_END();

    if (fbu_service_delete_user(svc->svh, username, username_len)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Service, backup)
{

    TODO("PHP_METHOD(FireBird_Service, backup)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, restore)
{
    TODO("PHP_METHOD(FireBird_Service, restore)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, shutdown_db)
{
    TODO("PHP_METHOD(FireBird_Service, shutdown_db)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, db_online)
{
    TODO("PHP_METHOD(FireBird_Service, db_online)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, set_page_buffers)
{
    TODO("PHP_METHOD(FireBird_Service, set_page_buffers)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, set_sweep_interval)
{
    TODO("PHP_METHOD(FireBird_Service, set_sweep_interval)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, deny_new_attachments)
{
    TODO("PHP_METHOD(FireBird_Service, deny_new_attachments)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, deny_new_transactions)
{
    TODO("PHP_METHOD(FireBird_Service, deny_new_transactions)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, set_write_mode_async)
{
    TODO("PHP_METHOD(FireBird_Service, set_write_mode_async)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, set_write_mode_sync)
{
    TODO("PHP_METHOD(FireBird_Service, set_write_mode_sync)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, set_access_mode_readonly)
{
    TODO("PHP_METHOD(FireBird_Service, set_access_mode_readonly)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, set_access_mode_readwrite)
{
    TODO("PHP_METHOD(FireBird_Service, set_access_mode_readwrite)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, enable_reserve_space)
{
    TODO("PHP_METHOD(FireBird_Service, enable_reserve_space)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, disable_reserve_space)
{
    TODO("PHP_METHOD(FireBird_Service, disable_reserve_space)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Service, set_sql_dialect)
{
    TODO("PHP_METHOD(FireBird_Service, set_sql_dialect)");
#if 0
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
#endif
}

static zend_object *FireBird_Service_create_object(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Service_create_object()");

    firebird_service *sv = (firebird_service *)zend_object_alloc(sizeof(firebird_service), ce);

    zend_object_std_init(&sv->std, ce);
    object_properties_init(&sv->std, ce);

    return &sv->std;
}

static void FireBird_Service_free_obj(zend_object *obj)
{
    firebird_service *sv = get_firebird_service_from_obj(obj);

    if (fbu_is_valid_svh(sv->svh)) {
        fbu_service_free(sv->svh);
        sv->svh = 0;
    }

    zend_object_std_dtor(&sv->std);
}

void register_FireBird_Service_object_handlers()
{
    FireBird_Service_ce->create_object = FireBird_Service_create_object;
    FireBird_Service_ce->default_object_handlers = &FireBird_Service_object_handlers;

    memcpy(&FireBird_Service_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Service_object_handlers.offset = XtOffsetOf(firebird_service, std);
    FireBird_Service_object_handlers.free_obj = FireBird_Service_free_obj;
}

}
