#include <ibase.h>
// #include "firebird/fb_c_api.h"
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Connection_ce;
static zend_object_handlers FireBird_Connection_object_handlers;

PHP_METHOD(Connection, close) {
    ZEND_PARSE_PARAMETERS_NONE();

    php_printf("Connection::close()\n");

    firebird_connection *conn = Z_CONNECTION_P(ZEND_THIS);

    if(conn->db_handle) {
        ISC_STATUS_ARRAY status;
        php_printf("Closing handle: %d\n", conn->db_handle);
        if(isc_detach_database(status, &conn->db_handle)){
            update_err_props(status, FireBird_Connection_ce, Z_OBJ_P(ZEND_THIS));
        } else {
            RETURN_TRUE;
        }
    }

    RETURN_FALSE;
}

// #define dpb_insert_int(tag, value) IXpbBuilder_insertInt(dpb, st, tag, value)
// #define dpb_insert_string(tag, value) IXpbBuilder_insertString(dpb, st, tag, value)
// #define dpb_insert_tag(tag) IXpbBuilder_insertTag(dpb, st, tag)

// PHP_METHOD(Connection, create) {
//     firebird_connection *conn = Z_CONNECTION_O(ZEND_THIS);
//     ISC_STATUS_ARRAY status;
//     // isc_tr_handle tr = 0;

//     struct IMaster* master = fb_get_master_interface();
//     struct IStatus* st = IMaster_getStatus(master);
//     // struct IProvider* prov = IMaster_getDispatcher(master);
//     struct IUtil* utl = IMaster_getUtilInterface(master);
//     struct IXpbBuilder* dpb = NULL;

//     dpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_DPB, NULL, 0);

//     // dpb_insert_tag(isc_dpb_version1);
//     dpb_insert_int(isc_dpb_page_size, 4096);
//     // dpb_insert_string(isc_dpb_user_name, "sysdba");
//     // dpb_insert_string(isc_dpb_password, "masterkey");

//     dump_buffer((unsigned char *)IXpbBuilder_getBuffer(dpb, st), IXpbBuilder_getBufferLength(dpb, st));

//     // isc_dpb_password
//     // isc_create_database
//     // if (isc_dsql_execute_immediate(status, &conn->handle, &tr, 0, create_db, 1,
//     //     NULL))
//     RETURN_FALSE;
// }

PHP_METHOD(Connection, start_transaction) {
    object_init_ex(return_value, FireBird_Transaction_ce);

    zend_long trans_args = 0, lock_timeout = 0;
    bool trans_args_is_null = 1, lock_timeout_is_null = 1;

    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(trans_args)
        Z_PARAM_LONG(lock_timeout)
    ZEND_PARSE_PARAMETERS_END();

    transaction_ctor(return_value, ZEND_THIS, trans_args, lock_timeout);
    if(FAILURE == transaction_start(return_value)) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

const zend_function_entry FireBird_Connection_methods[] = {
    PHP_ME(Connection, close, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Connection, start_transaction, arginfo_FireBird_Connection_start_transaction, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Connection_create(zend_class_entry *ce)
{
    php_printf("FireBird_Connection_create\n");
    firebird_connection *conn = zend_object_alloc(sizeof(firebird_connection), ce);

    zend_object_std_init(&conn->std, ce);
    object_properties_init(&conn->std, ce);

    conn->db_handle = 0;

    return &conn->std;
}

static void FireBird_Connection_free_obj(zend_object *obj)
{
    php_printf("FireBird_Connection_free_obj\n");
    firebird_connection *conn = Z_CONNECTION_O(obj);

    if(conn->db_handle) {
        ISC_STATUS_ARRAY status;
        if(isc_detach_database(status, &conn->db_handle)) {
            // TODO: report errors?
        }
        conn->db_handle = 0;
    }

    zend_object_std_dtor(&conn->std);
}

void register_FireBird_Connection_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Connection", FireBird_Connection_methods);
    FireBird_Connection_ce = zend_register_internal_class(&tmp_ce);

    ADD_ERR_PROPS(FireBird_Connection_ce);

    FireBird_Connection_ce->create_object = FireBird_Connection_create;
    FireBird_Connection_ce->default_object_handlers = &FireBird_Connection_object_handlers;

    memcpy(&FireBird_Connection_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Connection_object_handlers.offset = XtOffsetOf(firebird_connection, std);
    FireBird_Connection_object_handlers.free_obj = FireBird_Connection_free_obj;
}
