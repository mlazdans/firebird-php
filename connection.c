#include <ibase.h>
// #include "firebird/fb_c_api.h"
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Connection_ce;
static zend_object_handlers FireBird_Connection_object_handlers;

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

void connection_ctor(zval *conn_o, zval *database)
{
    zend_update_property(FireBird_Connection_ce, O_SET(conn_o, database));
}

PHP_METHOD(Connection, __construct) {
    zval *database = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(database, FireBird_Database_ce)
    ZEND_PARSE_PARAMETERS_END();

    connection_ctor(ZEND_THIS, database);
}

int connection_connect(ISC_STATUS_ARRAY status, zval *conn_o)
{
    zval rv, *database, *db_o, *val;

    db_o = zend_read_property(FireBird_Connection_ce, O_GET(conn_o, database), 1, &rv);
    database = zend_read_property(FireBird_Database_ce, O_GET(db_o, database), 1, &rv);

    if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
        return FAILURE;
    }

    firebird_connection *conn = Z_CONNECTION_P(conn_o);

    php_printf("connection_connect: %s\n", Z_STRVAL_P(database));

    long SQLCODE;

    static char const dpb_args_str[] = { isc_dpb_user_name, isc_dpb_password, isc_dpb_lc_ctype, isc_dpb_sql_role_name };
    const char *class_args_str[] = { "username", "password", "charset", "role" };

    char dpb_buffer[257] = {0}, *dpb;
    short dpb_len, buf_len = sizeof(dpb_buffer);

    dpb = dpb_buffer;

    // TODO: isc_dpb_version2
    *dpb++ = isc_dpb_version1; buf_len--;

    int len = sizeof(class_args_str) / sizeof(class_args_str[0]);
    for(int i = 0; i < len; i++){
        val = zend_read_property(FireBird_Database_ce, Z_OBJ_P(db_o), class_args_str[i], strlen(class_args_str[i]), 1, &rv);
        if (Z_TYPE_P(val) == IS_STRING && Z_STRLEN_P(val)) {
            php_printf("arg%d: %s = %s\n", i, class_args_str[i], Z_STRVAL_P(val));
            dpb_len = slprintf(dpb, buf_len, "%c%c%s", dpb_args_str[i], (unsigned char)Z_STRLEN_P(val), Z_STRVAL_P(val));
            dpb += dpb_len;
            buf_len -= dpb_len;
        }
    }

    val = zend_read_property(FireBird_Database_ce, Z_OBJ_P(db_o), "buffers", sizeof("buffers") - 1, 1, &rv);
    if (!Z_ISNULL_P(val)) {
        dpb_len = slprintf(dpb, buf_len, "%c\2%c%c", isc_dpb_num_buffers,
            (char)(Z_LVAL_P(val) >> 8), (char)(Z_LVAL_P(val) & 0xff));
        dpb += dpb_len;
        buf_len -= dpb_len;
    }

    // TODO: something does not add up here. isc_spb_prp_wm_sync is related to services, why the val == isc_spb_prp_wm_sync check?
    //       Disabling for now
    // val = zend_read_property(FireBird_Database_ce, Z_OBJ_P(ZEND_THIS), "sync", sizeof("sync") - 1, 1, &rv);
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

    if (isc_attach_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &conn->db_handle, (short)(dpb-dpb_buffer), dpb_buffer)) {
        return FAILURE;
    }

    php_printf("Connected, handle: %d\n", conn->db_handle);

    return SUCCESS;
}

PHP_METHOD(Connection, connect) {
    ZEND_PARSE_PARAMETERS_NONE();

    ISC_STATUS_ARRAY status;

    if (FAILURE == connection_connect(status, ZEND_THIS)) {
        update_err_props(status, FireBird_Connection_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(Connection, disconnect) {
    ZEND_PARSE_PARAMETERS_NONE();

    php_printf("Connection::disconnect()\n");

    firebird_connection *conn = Z_CONNECTION_P(ZEND_THIS);

    if(conn->db_handle) {
        ISC_STATUS_ARRAY status;
        php_printf("Closing handle: %d\n", conn->db_handle);
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

    DECLARE_PROP_OBJ(FireBird_Connection_ce, database, FireBird\\Database, ZEND_ACC_PROTECTED_SET);
    ADD_ERR_PROPS(FireBird_Connection_ce);

    FireBird_Connection_ce->create_object = FireBird_Connection_create;
    FireBird_Connection_ce->default_object_handlers = &FireBird_Connection_object_handlers;

    memcpy(&FireBird_Connection_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Connection_object_handlers.offset = XtOffsetOf(firebird_connection, std);
    FireBird_Connection_object_handlers.free_obj = FireBird_Connection_free_obj;
}
