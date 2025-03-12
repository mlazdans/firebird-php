// #include <ibase.h>
#include "firebird/fb_c_api.h"
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Database_ce;
static zend_object_handlers FireBird_Database_object_handlers;

// PHP_METHOD(Database, __construct)
// {
//     zend_string *database = NULL, *username = NULL, *password = NULL, *charset = NULL, *role = NULL;
//     zend_long num_buffers = 0, sweep_interval = 0, set_page_buffers = 0, page_size = 0, force_write = 0;
//     bool num_buffers_is_null = 1, sweep_interval_is_null = 1, set_page_buffers_is_null = 1, page_size_is_null = 1, force_write_is_null = 1;

//     ZEND_PARSE_PARAMETERS_START(1, 10)
//         Z_PARAM_STR(database)
//         Z_PARAM_OPTIONAL
//         Z_PARAM_STR_OR_NULL(username)
//         Z_PARAM_STR_OR_NULL(password)
//         Z_PARAM_STR_OR_NULL(charset)
//         Z_PARAM_LONG_OR_NULL(num_buffers, num_buffers_is_null)
//         Z_PARAM_STR_OR_NULL(role)
//         Z_PARAM_LONG_OR_NULL(sweep_interval, sweep_interval_is_null)
//         Z_PARAM_LONG_OR_NULL(set_page_buffers, set_page_buffers_is_null)
//         Z_PARAM_LONG_OR_NULL(page_size, page_size_is_null)
//         Z_PARAM_LONG_OR_NULL(force_write, force_write_is_null)
//     ZEND_PARSE_PARAMETERS_END();

//     zend_update_property_str(FireBird_Database_ce, THIS_SET(database));

//     if (username) {
//         zend_update_property_str(FireBird_Database_ce, THIS_SET(username));
//     }

//     if (password) {
//         zend_update_property_str(FireBird_Database_ce, THIS_SET(password));
//     }

//     if (charset) {
//         zend_update_property_str(FireBird_Database_ce, THIS_SET(charset));
//     }

//     if (!num_buffers_is_null) {
//         zend_update_property_long(FireBird_Database_ce, THIS_SET(num_buffers));
//     }

//     if (role) {
//         zend_update_property_str(FireBird_Database_ce, THIS_SET(role));
//     }

//     if (!sweep_interval_is_null) {
//         zend_update_property_long(FireBird_Database_ce, THIS_SET(sweep_interval));
//     }

//     if (!set_page_buffers_is_null) {
//         zend_update_property_long(FireBird_Database_ce, THIS_SET(set_page_buffers));
//     }

//     if (!page_size_is_null) {
//         zend_update_property_long(FireBird_Database_ce, THIS_SET(page_size));
//     }

//     if (!force_write_is_null) {
//         zend_update_property_long(FireBird_Database_ce, THIS_SET(force_write));
//     }
// }

// PHP_METHOD(Database, create)
// {
//     zval rv, *database, *db_o;
//     ISC_STATUS_ARRAY status;
//     char dpb_buffer[257] = {0};
//     short num_dpb_written;

//     if (FAILURE == database_build_dpb(status, ZEND_THIS, dpb_buffer, sizeof(dpb_buffer), &num_dpb_written)) {
//         update_err_props(status, FireBird_Database_ce, ZEND_THIS);
//         RETURN_FALSE;
//     }

//     database = zend_read_property(FireBird_Database_ce, THIS_GET(database), 1, &rv);
//     FBDEBUG("create: %s", Z_STRVAL_P(database));

//     isc_db_handle db_h = 0;
//     if (isc_create_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &db_h, num_dpb_written, dpb_buffer, 0)) {
//         update_err_props(status, FireBird_Database_ce, ZEND_THIS);
//         RETURN_FALSE;
//     }

//     RETURN_TRUE;
// }

const zend_function_entry FireBird_Database_methods[] = {
    // PHP_ME(Database, __construct, arginfo_FireBird_Database_construct, ZEND_ACC_PUBLIC)
    // PHP_ME(Database, create, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    // PHP_ME(Database, connect, arginfo_FireBird_Database_connect, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Database_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Database_create");

    firebird_db *db = zend_object_alloc(sizeof(firebird_db), ce);

    zend_object_std_init(&db->std, ce);
    object_properties_init(&db->std, ce);

    return &db->std;
}

static void FireBird_Database_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Database_free_obj");

    firebird_db *db = Z_DB_O(obj);

    zend_object_std_dtor(&db->std);
}

void register_FireBird_Database_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Database", FireBird_Database_methods);
    FireBird_Database_ce = zend_register_internal_class(&tmp_ce);

    // DECLARE_PROP_STRING(FireBird_Database_ce, database, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_STRING(FireBird_Database_ce, username, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_STRING(FireBird_Database_ce, password, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_STRING(FireBird_Database_ce, charset, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_LONG(FireBird_Database_ce, num_buffers, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_STRING(FireBird_Database_ce, role, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_LONG(FireBird_Database_ce, sweep_interval, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_LONG(FireBird_Database_ce, set_page_buffers, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_LONG(FireBird_Database_ce, page_size, ZEND_ACC_PROTECTED_SET);
    // DECLARE_PROP_BOOL(FireBird_Database_ce, force_write, ZEND_ACC_PROTECTED_SET);
    // DECLARE_ERR_PROPS(FireBird_Database_ce);

    // zend_class_implements(FireBird_Database_ce, 1, FireBird_IError_ce);

    // Sensitive attribute can't be added to properties. Maybe someday
    zend_add_parameter_attribute(zend_hash_str_find_ptr(&FireBird_Database_ce->function_table,
        "__construct", sizeof("__construct") - 1), 2, ZSTR_KNOWN(ZEND_STR_SENSITIVEPARAMETER), 0);

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
