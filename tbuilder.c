#include <ibase.h>
#include <firebird/fb_c_api.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_TBuilder_ce;
static zend_object_handlers FireBird_TBuilder_object_handlers;

#define tbuilder_update_flag(name) do {                   \
    zend_bool enable = true;                              \
    ZEND_PARSE_PARAMETERS_START(0, 1)                     \
        Z_PARAM_OPTIONAL                                  \
        Z_PARAM_BOOL(enable)                              \
    ZEND_PARSE_PARAMETERS_END();                          \
    firebird_tbuilder *builder = Z_TBUILDER_P(ZEND_THIS); \
    builder->name = (bool)enable;                         \
} while(0)

#define tbuilder_update_isolation_mode(mode)              \
    firebird_tbuilder *builder = Z_TBUILDER_P(ZEND_THIS); \
    builder->isolation_mode = mode

PHP_METHOD(TBuilder, __construct)
{
    tbuilder_update_isolation_mode(1);
}

PHP_METHOD(TBuilder, read_only)
{
    tbuilder_update_flag(read_only);
}

PHP_METHOD(TBuilder, ignore_limbo)
{
    tbuilder_update_flag(ignore_limbo);
}

PHP_METHOD(TBuilder, auto_commit)
{
    tbuilder_update_flag(auto_commit);
}

PHP_METHOD(TBuilder, no_auto_undo)
{
    tbuilder_update_flag(no_auto_undo);
}

PHP_METHOD(TBuilder, wait)
{
    zend_long lock_timeout = -1;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(lock_timeout)
    ZEND_PARSE_PARAMETERS_END();

    if (lock_timeout < -1 || lock_timeout > 0x7FFF) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "lock_timeout argument must be between -1 and %d", 0x7FFF);
        RETURN_THROWS();
    }

    firebird_tbuilder *builder = Z_TBUILDER_O(ZEND_THIS);
    builder->lock_timeout = lock_timeout;
}

PHP_METHOD(TBuilder, isolation_snapshot_table_stability)
{
    tbuilder_update_isolation_mode(0);
}

PHP_METHOD(TBuilder, isolation_snapshot)
{
    zend_long snapshot_at_number = 0;
    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(snapshot_at_number)
    ZEND_PARSE_PARAMETERS_END();

    tbuilder_update_isolation_mode(1);
    builder->snapshot_at_number = snapshot_at_number;
}

PHP_METHOD(TBuilder, isolation_read_committed_record_version)
{
    tbuilder_update_isolation_mode(2);
}

PHP_METHOD(TBuilder, isolation_read_committed_no_record_version)
{
    tbuilder_update_isolation_mode(3);
}

PHP_METHOD(TBuilder, isolation_read_committed_read_consistency)
{
    tbuilder_update_isolation_mode(4);
}

const zend_function_entry FireBird_TBuilder_methods[] = {
    PHP_ME(TBuilder, __construct, arginfo_none, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, read_only, arginfo_bool_return_none, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, ignore_limbo, arginfo_bool_return_none, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, auto_commit, arginfo_bool_return_none, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, no_auto_undo, arginfo_bool_return_none, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, wait, arginfo_FireBird_TBuilder_wait, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_snapshot_table_stability, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_snapshot, arginfo_FireBird_TBuilder_isolation_snapshot, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_read_committed_record_version, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_read_committed_no_record_version, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_read_committed_read_consistency, arginfo_void, ZEND_ACC_PUBLIC)

    PHP_FE_END
};

static zend_object *FireBird_TBuilder_create(zend_class_entry *ce)
{
    firebird_tbuilder *builder = zend_object_alloc(sizeof(firebird_tbuilder), ce);

    zend_object_std_init(&builder->std, ce);
    object_properties_init(&builder->std, ce);

    return &builder->std;
}

static void FireBird_TBuilder_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_TBuilder_free_obj");

    firebird_tbuilder *builder = Z_TBUILDER_O(obj);

    zend_object_std_dtor(&builder->std);
}

void register_FireBird_TBuilder_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "TBuilder", FireBird_TBuilder_methods);
    FireBird_TBuilder_ce = zend_register_internal_class(&tmp_ce);

    FireBird_TBuilder_ce->create_object = FireBird_TBuilder_create;
    FireBird_TBuilder_ce->default_object_handlers = &FireBird_TBuilder_object_handlers;

    memcpy(&FireBird_TBuilder_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_TBuilder_object_handlers.offset = XtOffsetOf(firebird_tbuilder, std);
    FireBird_TBuilder_object_handlers.free_obj = FireBird_TBuilder_free_obj;
}
