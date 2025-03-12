#include <ibase.h>
#include "firebird/fb_c_api.h"
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Database_ce;
static zend_object_handlers FireBird_Database_object_handlers;

PHP_METHOD(Database, __construct) {
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

    zend_update_property_str(FireBird_Database_ce, THIS_SET(database));

    if(username) {
        zend_update_property_str(FireBird_Database_ce, THIS_SET(username));
    }

    if(password) {
        zend_update_property_str(FireBird_Database_ce, THIS_SET(password));
    }

    if(charset) {
        zend_update_property_str(FireBird_Database_ce, THIS_SET(charset));
    }

    if(!buffers_is_null) {
        zend_update_property_long(FireBird_Database_ce, THIS_SET(buffers));
    }

    if(!dialect_is_null) {
        zend_update_property_long(FireBird_Database_ce, THIS_SET(dialect));
    }

    if(role) {
        zend_update_property_str(FireBird_Database_ce, THIS_SET(role));
    }
}

const zend_function_entry FireBird_Database_methods[] = {
    PHP_ME(Database, __construct, arginfo_FireBird_Database_construct, ZEND_ACC_PUBLIC)
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

    DECLARE_PROP_STRING(FireBird_Database_ce, database, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_STRING(FireBird_Database_ce, username, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_STRING(FireBird_Database_ce, password, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_STRING(FireBird_Database_ce, charset, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_INT(FireBird_Database_ce, buffers, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_INT(FireBird_Database_ce, dialect, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_STRING(FireBird_Database_ce, role, ZEND_ACC_PROTECTED_SET);

    ADD_ERR_PROPS(FireBird_Database_ce);

    // Sensitive attribute can't be added to properties. Maybe someday
    zend_add_parameter_attribute(zend_hash_str_find_ptr(&FireBird_Database_ce->function_table,
        "__construct", sizeof("__construct") - 1), 2, ZSTR_KNOWN(ZEND_STR_SENSITIVEPARAMETER), 0);

    FireBird_Database_ce->create_object = FireBird_Database_create;
    FireBird_Database_ce->default_object_handlers = &FireBird_Database_object_handlers;

    memcpy(&FireBird_Database_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Database_object_handlers.offset = XtOffsetOf(firebird_db, std);
    FireBird_Database_object_handlers.free_obj = FireBird_Database_free_obj;
}
