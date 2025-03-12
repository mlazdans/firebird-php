#include <ibase.h>
// #include "firebird/fb_c_api.h"
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Connection_ce;
static zend_object_handlers FireBird_Connection_object_handlers;

// PHP_METHOD(Connection, start_transaction) {
//     zend_long trans_args = 0, lock_timeout = 0;
//     ISC_STATUS_ARRAY status;

//     ZEND_PARSE_PARAMETERS_START(0, 2)
//         Z_PARAM_OPTIONAL
//         Z_PARAM_LONG(trans_args)
//         Z_PARAM_LONG(lock_timeout)
//     ZEND_PARSE_PARAMETERS_END();

//     object_init_ex(return_value, FireBird_Transaction_ce);

//     transaction_ctor(return_value, ZEND_THIS, trans_args, lock_timeout);
//     if(FAILURE == transaction_start(status, return_value)) {
//         zval_ptr_dtor(return_value);
//         update_err_props(status, FireBird_Connection_ce, ZEND_THIS);
//         RETURN_FALSE;
//     }
// }

void connection_ctor(zval *conn_o, zval *args)
{
    zend_update_property(FireBird_Connection_ce, O_SET(conn_o, args));
}

PHP_METHOD(Connection, __construct) {
    zval *args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(args, FireBird_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    connection_ctor(ZEND_THIS, args);
}

int connection_connect(ISC_STATUS_ARRAY status, zval *conn_o)
{
    zval rv, *args, *args_o, *database;
    firebird_connection *conn = Z_CONNECTION_P(conn_o);
    const char *dpb_buffer;
    short num_dpb_written;

    firebird_xpb_args map = {
        .tags = (const char[]) {
            isc_dpb_user_name, isc_dpb_password, isc_dpb_lc_ctype, isc_dpb_sql_role_name, isc_dpb_num_buffers
        },
        .names = (const char *[]) {
            "username", "password", "charset", "role", "buffers"
        },
        .count = 5
    };

    args_o = zend_read_property(FireBird_Connection_ce, O_GET(conn_o, args), 1, &rv);
    database = zend_read_property(FireBird_Connect_Args_ce, O_GET(args_o, database), 1, &rv);

    if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
        return FAILURE;
    }

    if (FAILURE == database_build_dpb(FireBird_Connect_Args_ce, args_o, &map, &dpb_buffer, &num_dpb_written)) {
        return FAILURE;
    }

    FBDEBUG("connection_connect: %s", Z_STRVAL_P(database));
    if (isc_attach_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &conn->db_handle, num_dpb_written, dpb_buffer)) {
        return FAILURE;
    }
    FBDEBUG("Connected, handle: %d", conn->db_handle);

    return SUCCESS;
}

PHP_METHOD(Connection, connect) {
    ZEND_PARSE_PARAMETERS_NONE();

    ISC_STATUS_ARRAY status = {0};

    if (FAILURE == connection_connect(status, ZEND_THIS)) {
        update_err_props(status, FireBird_Connection_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(Connection, disconnect) {
    ZEND_PARSE_PARAMETERS_NONE();

    FBDEBUG("Connection::disconnect()");

    firebird_connection *conn = Z_CONNECTION_P(ZEND_THIS);

    if(conn->db_handle) {
        ISC_STATUS_ARRAY status;
        FBDEBUG("Closing handle: %d", conn->db_handle);
        if(isc_detach_database(status, &conn->db_handle)){
            update_err_props(status, FireBird_Connection_ce, ZEND_THIS);
        } else {
            conn->db_handle = 0;
            RETURN_TRUE;
        }
    }

    RETURN_FALSE;
}

const zend_function_entry FireBird_Connection_methods[] = {
    PHP_ME(Connection, __construct, arginfo_FireBird_Connection_construct, ZEND_ACC_PUBLIC)
    PHP_ME(Connection, connect, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Connection, disconnect, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    // PHP_ME(Connection, start_transaction, arginfo_FireBird_Connection_start_transaction, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Connection_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Connection_create");

    firebird_connection *conn = zend_object_alloc(sizeof(firebird_connection), ce);

    zend_object_std_init(&conn->std, ce);
    object_properties_init(&conn->std, ce);

    conn->db_handle = 0;

    return &conn->std;
}

static void FireBird_Connection_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Connection_free_obj");

    firebird_connection *conn = Z_CONNECTION_O(obj);

    // TODO: code dup with disconnect()
    if(conn->db_handle) {
        ISC_STATUS_ARRAY status;
        if(isc_detach_database(status, &conn->db_handle)) {
            // TODO: report errors?
        } else {
            conn->db_handle = 0;
        }
    }

    zend_object_std_dtor(&conn->std);
}

void register_FireBird_Connection_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Connection", FireBird_Connection_methods);
    FireBird_Connection_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Connection_ce, args, FireBird\\Connect_Args, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Connection_ce);

    zend_class_implements(FireBird_Connection_ce, 1, FireBird_IError_ce);

    FireBird_Connection_ce->create_object = FireBird_Connection_create;
    FireBird_Connection_ce->default_object_handlers = &FireBird_Connection_object_handlers;

    memcpy(&FireBird_Connection_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Connection_object_handlers.offset = XtOffsetOf(firebird_connection, std);
    FireBird_Connection_object_handlers.free_obj = FireBird_Connection_free_obj;
}
