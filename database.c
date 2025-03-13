// #include <ibase.h>
#include "firebird/fb_c_api.h"
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Database_ce;
static zend_object_handlers FireBird_Database_object_handlers;

int database_connect(ISC_STATUS_ARRAY status, zval *db_o, zval *args_o)
{
    zval rv, *database;
    const char *dpb_buffer;
    short num_dpb_written;
    firebird_db *db = Z_DB_P(db_o);

    firebird_xpb_args map = {
        .tags = (const char[]) {
            isc_dpb_user_name, isc_dpb_password, isc_dpb_lc_ctype, isc_dpb_sql_role_name, isc_dpb_num_buffers
        },
        .names = (const char *[]) {
            "username", "password", "charset", "role", "buffers"
        },
        .count = 5
    };

    database = zend_read_property(FireBird_Connect_Args_ce, O_GET(args_o, database), 1, &rv);

    if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
        return FAILURE;
    }

    if (FAILURE == database_build_dpb(FireBird_Connect_Args_ce, args_o, &map, &dpb_buffer, &num_dpb_written)) {
        return FAILURE;
    }

    FBDEBUG("connection_connect: %s", Z_STRVAL_P(database));
    if (isc_attach_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &db->db_handle, num_dpb_written, dpb_buffer)) {
        return FAILURE;
    }
    FBDEBUG("Connected, handle: %d", db->db_handle);

    return SUCCESS;
}

PHP_METHOD(Database, connect)
{
    zval *args = NULL;
    ISC_STATUS_ARRAY status = {0};

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(args, FireBird_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == database_connect(status, ZEND_THIS, args)) {
        RETURN_FALSE;
    }

    object_init_ex(return_value, FireBird_Connection_ce);
    connection_ctor(return_value, ZEND_THIS);
    zend_update_property(FireBird_Connection_ce, O_SET(return_value, args));
}

// Flags used by createDatabase() jrd/jrd.cpp
// dpb_sweep_interval
// dpb_length
// dpb_auth_block
// dpb_sql_dialect
// dpb_org_filename
// dpb_utf8_filename
// dpb_owner
// dpb_remote_address
// dpb_working_directory
// dpb_sec_attach
// dpb_map_attach
// dpb_gbak_attach
// dpb_no_db_triggers
// dpb_interp
// dpb_page_size
// dpb_overwrite
// dpb_set_page_buffers
// dpb_set_no_reserve
// dpb_set_db_charset
// dpb_online
// dpb_shutdown
// dpb_activate_shadow
// dpb_parallel_workers
// dpb_set_db_readonly
// dpb_set_db_replica
// dpb_replica_mode
// dpb_session_tz
// dpb_set_force_write
// dpb_force_write
// int database_create(ISC_STATUS_ARRAY status, zval *conn_o)
// {
//     zval rv, *args, *args_o, *database;
//     firebird_connection *conn = Z_CONNECTION_P(conn_o);
//     const char *dpb_buffer;
//     short num_dpb_written;

//     firebird_xpb_args map = {
//         .tags = (const char[]) {
//             isc_dpb_user_name, isc_dpb_password, isc_dpb_set_db_charset, isc_dpb_sweep_interval, isc_dpb_set_page_buffers
//         },
//         .names = (const char *[]) {
//             "username", "password", "charset", "sweep_interval", "buffers", "page_size", "force_write"
//         },
//         .count = 5
//     };

//     args_o = zend_read_property(FireBird_Connection_ce, O_GET(conn_o, args), 1, &rv);
//     database = zend_read_property(FireBird_Create_Args_ce, O_GET(args_o, database), 1, &rv);

//     if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
//         zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
//         return FAILURE;
//     }

//     if (FAILURE == database_build_dpb(FireBird_Create_Args_ce, args_o, &map, &dpb_buffer, &num_dpb_written)) {
//         return FAILURE;
//     }

//     FBDEBUG("database_create: %s", Z_STRVAL_P(database));
//     if (isc_create_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &conn->db_handle, num_dpb_written, dpb_buffer, 0)) {
//         return FAILURE;
//     }
//     FBDEBUG("Created, handle: %d", conn->db_handle);

//     return SUCCESS;
// }

PHP_METHOD(Database, create)
{
    php_error_docref(NULL, E_WARNING, "TODO: Database::create()");
    RETURN_FALSE;
    // zval rv, *database, *db_o;
    // ISC_STATUS_ARRAY status;
    // char dpb_buffer[257] = {0};
    // short num_dpb_written;

    // if (FAILURE == database_build_dpb(status, ZEND_THIS, dpb_buffer, sizeof(dpb_buffer), &num_dpb_written)) {
    //     update_err_props(status, FireBird_Database_ce, ZEND_THIS);
    //     RETURN_FALSE;
    // }

    // database = zend_read_property(FireBird_Database_ce, THIS_GET(database), 1, &rv);
    // FBDEBUG("create: %s", Z_STRVAL_P(database));

    // isc_db_handle db_h = 0;
    // if (isc_create_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &db_h, num_dpb_written, dpb_buffer, 0)) {
    //     update_err_props(status, FireBird_Database_ce, ZEND_THIS);
    //     RETURN_FALSE;
    // }

    // RETURN_TRUE;
}

const zend_function_entry FireBird_Database_methods[] = {
    PHP_ME(Database, create, arginfo_FireBird_Database_create, ZEND_ACC_PUBLIC)
    PHP_ME(Database, connect, arginfo_FireBird_Database_connect, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Database_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Database_create");

    firebird_db *db = zend_object_alloc(sizeof(firebird_db), ce);
    db->db_handle = 0;

    zend_object_std_init(&db->std, ce);
    object_properties_init(&db->std, ce);

    return &db->std;
}

static void FireBird_Database_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Database_free_obj");

    firebird_db *db = Z_DB_O(obj);

    if(db->db_handle) {
        ISC_STATUS_ARRAY status;
        if(isc_detach_database(status, &db->db_handle)) {
            // TODO: test
            char msg[1024] = {0};
            status_err_msg(status, msg, sizeof(msg));
            _php_firebird_module_error(msg);
        } else {
            db->db_handle = 0;
        }
    }

    zend_object_std_dtor(&db->std);
}

void register_FireBird_Database_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Database", FireBird_Database_methods);
    FireBird_Database_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_ERR_PROPS(FireBird_Database_ce);

    zend_class_implements(FireBird_Database_ce, 1, FireBird_IError_ce);

    FireBird_Database_ce->create_object = FireBird_Database_create;
    FireBird_Database_ce->default_object_handlers = &FireBird_Database_object_handlers;

    memcpy(&FireBird_Database_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Database_object_handlers.offset = XtOffsetOf(firebird_db, std);
    FireBird_Database_object_handlers.free_obj = FireBird_Database_free_obj;
}

#define dpb_insert(f, ...) do { \
    IXpbBuilder_insert##f(__VA_ARGS__); \
    if (IStatus_getState(st) & IStatus_STATE_ERRORS) { \
        char msg[1024] = {0}; \
        status_err_msg(IStatus_getErrors(st), msg, sizeof(msg)); \
        _php_firebird_module_error(msg); \
        return FAILURE; \
    } \
} while(0)

#define dpb_insert_int(tag, value) dpb_insert(Int, dpb, st, tag, value)
#define dpb_insert_string(tag, value) dpb_insert(String, dpb, st, tag, value)
#define dpb_insert_tag(tag) dpb_insert(Tag, dpb, st, tag)

int database_build_dpb(zend_class_entry *ce, zval *args_o, firebird_xpb_args *xpb_args, const char **dpb_buf, short *num_dpb_written)
{
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* dpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_DPB, NULL, 0);
    zval rv, *val;
    int i;

    dpb_insert_tag(isc_dpb_version2);
    for (int i = 0; i < xpb_args->count; i++) {
        val = zend_read_property(ce, Z_OBJ_P(args_o), xpb_args->names[i], strlen(xpb_args->names[i]), 1, &rv);
        switch (Z_TYPE_P(val)) {
            case IS_STRING:
                FBDEBUG("property: %s is string: `%s`", xpb_args->names[i], Z_STRVAL_P(val));
                dpb_insert_string(xpb_args->tags[i], Z_STRVAL_P(val));
                break;
            case IS_LONG:
                FBDEBUG("property: %s is long: `%u`", xpb_args->names[i], Z_LVAL_P(val));
                dpb_insert_int(xpb_args->tags[i], (int)Z_LVAL_P(val));
                break;
            case IS_NULL:
                break;
            default:
                // TODO: It's a BUG, should print warning really
                FBDEBUG("unhandled: %s type %s", xpb_args->names[i], zend_get_type_by_const(Z_TYPE_P(val)));
                break;
        }
    }

#if FB_API_VER >= 40
    // Do not handle directly INT128 or DECFLOAT, convert to VARCHAR at server instead
    dpb_insert_string(isc_dpb_set_bind, "int128 to varchar;decfloat to varchar");
#endif

    // dump_buffer((unsigned char *)IXpbBuilder_getBuffer(dpb, st), IXpbBuilder_getBufferLength(dpb, st));

    *num_dpb_written = IXpbBuilder_getBufferLength(dpb, st);
    *dpb_buf = IXpbBuilder_getBuffer(dpb, st);

    return SUCCESS;
}
