#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *firebird_connection_ce;
static zend_object_handlers firebird_connection_object_handlers;

PHP_METHOD(Connection, connect) {
    ZEND_PARSE_PARAMETERS_NONE();

    php_printf("connect()\n");

    zval rv;
    zval *database, *val;

    long SQLCODE;
    ISC_STATUS_ARRAY status;
    firebird_db_link *conn = Z_CONNECTION_P(ZEND_THIS);

    static char const dpb_args_str[] = { isc_dpb_user_name, isc_dpb_password, isc_dpb_lc_ctype, isc_dpb_sql_role_name };
    const char *class_args_str[] = { "username", "password", "charset", "role" };

    database = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "database", sizeof("database") - 1, 1, &rv);
    if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
        RETURN_FALSE;
    }

    char dpb_buffer[257] = {0}, *dpb;
    short dpb_len, buf_len = sizeof(dpb_buffer);

    dpb = dpb_buffer;

    // TODO: isc_dpb_version2
    *dpb++ = isc_dpb_version1; buf_len--;

    int len = sizeof(class_args_str) / sizeof(class_args_str[0]);
    for(int i = 0; i < len; i++){
        val = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), class_args_str[i], strlen(class_args_str[i]), 1, &rv);
        if (Z_TYPE_P(val) == IS_STRING && Z_STRLEN_P(val)) {
            php_printf("arg%d: %s = %s\n", i, class_args_str[i], Z_STRVAL_P(val));
            dpb_len = slprintf(dpb, buf_len, "%c%c%s", dpb_args_str[i], (unsigned char)Z_STRLEN_P(val), Z_STRVAL_P(val));
            dpb += dpb_len;
            buf_len -= dpb_len;
        }
    }

    val = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "buffers", sizeof("buffers") - 1, 1, &rv);
    if (!Z_ISNULL_P(val)) {
        dpb_len = slprintf(dpb, buf_len, "%c\2%c%c", isc_dpb_num_buffers,
            (char)(Z_LVAL_P(val) >> 8), (char)(Z_LVAL_P(val) & 0xff));
        dpb += dpb_len;
        buf_len -= dpb_len;
    }

    // TODO: something does not add up here. isc_spb_prp_wm_sync is related to services, why the val == isc_spb_prp_wm_sync check?
    //       Disabling for now
    // val = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "sync", sizeof("sync") - 1, 1, &rv);
    // if (!Z_ISNULL_P(val)) {
    //     dpb_len = slprintf(dpb, buf_len, "%c\1%c", isc_dpb_force_write, Z_LVAL_P(val) == isc_spb_prp_wm_sync);
    //     dpb += dpb_len;
    //     buf_len -= dpb_len;
    // }

#if FB_API_VER >= 40
    // Do not handle directly INT128 or DECFLOAT, convert to VARCHAR at server instead
    const char *compat = "int128 to varchar;decfloat to varchar";
    dpb_len = slprintf(dpb, buf_len, "%c%c%s", isc_dpb_set_bind, strlen(compat), compat);
    dpb += dpb_len;
    buf_len -= dpb_len;
#endif

    if (isc_attach_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &conn->handle, (short)(dpb-dpb_buffer), dpb_buffer)) {
        update_err_props(status, firebird_connection_ce, Z_OBJ_P(ZEND_THIS));
        RETURN_FALSE;
    }

    php_printf("Connected!\n");

    RETURN_TRUE;
}

// PHP_METHOD(Connection, __destruct) {
//     zval rv;
//     zval *error_msg = zend_read_property(firebird_connection_ce, Z_OBJ_P(ZEND_THIS), "error_msg", sizeof("error_msg") - 1, 1, &rv);
//     if(!Z_ISNULL_P(error_msg)) {
//         zval_delref_p(error_msg);
//     }
// }

PHP_METHOD(Connection, __construct) {
    zend_string *database = NULL, *username = NULL, *password = NULL, *charset = NULL, *role = NULL;
    zend_long buffers = 0, dialect = 0;
    bool buffers_is_null = 1, dialect_is_null = 1;

    ZEND_PARSE_PARAMETERS_START(1, 7)
        Z_PARAM_STR(database)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(username)
        Z_PARAM_STR_OR_NULL(password)
        Z_PARAM_STR_OR_NULL(charset)
        Z_PARAM_LONG_OR_NULL(buffers, buffers_is_null)
        Z_PARAM_LONG_OR_NULL(dialect, dialect_is_null)
        Z_PARAM_STR_OR_NULL(role)
    ZEND_PARSE_PARAMETERS_END();

    if(database){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "database", sizeof("database") - 1, database);
        php_printf("database: %s\n", ZSTR_VAL(database));
    }

    if(username){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "username", sizeof("username") - 1, username);
        php_printf("username: %s\n", ZSTR_VAL(username));
    }

    if(password){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "password", sizeof("password") - 1, password);
        php_printf("password: %s\n", ZSTR_VAL(password));
    }

    if(charset){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "charset", sizeof("charset") - 1, charset);
        php_printf("charset: %s\n", ZSTR_VAL(charset));
    }

    if(!buffers_is_null){
        zend_update_property_long(firebird_connection_ce, Z_OBJ_P(getThis()), "buffers", sizeof("buffers") - 1, buffers);
        php_printf("buffers: %d\n", buffers);
    }

    if(!dialect_is_null){
        zend_update_property_long(firebird_connection_ce, Z_OBJ_P(getThis()), "dialect", sizeof("dialect") - 1, dialect);
        php_printf("dialect: %d\n", dialect);
    }

    if(role){
        zend_update_property_str(firebird_connection_ce, Z_OBJ_P(getThis()), "role", sizeof("role") - 1, role);
        php_printf("role: %s\n", ZSTR_VAL(role));
    }
}

const zend_function_entry firebird_connection_methods[] = {
    PHP_ME(Connection, __construct, arginfo_firebird_construct, ZEND_ACC_PUBLIC)
    // PHP_ME(Connection, __destruct, arginfo_firebird_void, ZEND_ACC_PUBLIC)
    PHP_ME(Connection, connect, arginfo_firebird_bool, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *firebird_connection_create(zend_class_entry *ce)
{
    php_printf("firebird_connection_create\n");
    firebird_db_link *conn = zend_object_alloc(sizeof(firebird_db_link), ce);

    zend_object_std_init(&conn->std, ce);
    object_properties_init(&conn->std, ce);

    conn->handle = 0;

    return &conn->std;
}

static void firebird_connection_free_obj(zend_object *obj)
{
    php_printf("firebird_connection_free_obj\n");
    firebird_db_link *conn = Z_CONNECTION_O(obj);

    // zval rv;
    // zval *error_msg = zend_read_property(firebird_connection_ce, obj, "error_msg", sizeof("error_msg") - 1, 1, &rv);
    // if(!Z_ISNULL_P(error_msg)) {
    //     zval_delref_p(error_msg);
    // }

    if(conn->handle) {
        ISC_STATUS_ARRAY status;
        php_printf("Closing handle: %d\n", conn->handle);
        // TODO: report errors?
        isc_detach_database(status, &conn->handle);
    }

    zend_object_std_dtor(&conn->std);
}

void firebird_register_connection_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Connection", firebird_connection_methods);
    firebird_connection_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_STRING(firebird_connection_ce, database, ZEND_ACC_PROTECTED);
    DECLARE_PROP_STRING(firebird_connection_ce, username, ZEND_ACC_PROTECTED);
    DECLARE_PROP_STRING(firebird_connection_ce, password, ZEND_ACC_PROTECTED);
    DECLARE_PROP_STRING(firebird_connection_ce, charset, ZEND_ACC_PROTECTED);
    DECLARE_PROP_INT(firebird_connection_ce, buffers, ZEND_ACC_PROTECTED);
    DECLARE_PROP_INT(firebird_connection_ce, dialect, ZEND_ACC_PROTECTED);
    DECLARE_PROP_STRING(firebird_connection_ce, role, ZEND_ACC_PROTECTED);

    ADD_ERR_PROPS(firebird_connection_ce);

    // Sensitive attribute can't be added to properties. Maybe someday
    zend_add_parameter_attribute(zend_hash_str_find_ptr(&firebird_connection_ce->function_table,
        "__construct", sizeof("__construct") - 1), 2, ZSTR_KNOWN(ZEND_STR_SENSITIVEPARAMETER), 0);

    firebird_connection_ce->create_object = firebird_connection_create;
    firebird_connection_ce->default_object_handlers = &firebird_connection_object_handlers;

    memcpy(&firebird_connection_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    firebird_connection_object_handlers.offset = XtOffsetOf(firebird_db_link, std);
    firebird_connection_object_handlers.free_obj = firebird_connection_free_obj;
}
