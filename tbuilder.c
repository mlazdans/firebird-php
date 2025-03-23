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
    RETVAL_OBJ_COPY(Z_OBJ_P(ZEND_THIS));                  \
} while(0)

#define tbuilder_update_isolation_mode(mode)              \
    firebird_tbuilder *builder = Z_TBUILDER_P(ZEND_THIS); \
    builder->isolation_mode = mode;                       \
    builder->snapshot_at_number = 0;                      \
    RETVAL_OBJ_COPY(Z_OBJ_P(ZEND_THIS))

PHP_METHOD(TBuilder, __construct)
{
    tbuilder_update_isolation_mode(1);
    builder->lock_timeout = -1;
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

    firebird_tbuilder *builder = Z_TBUILDER_P(ZEND_THIS);
    builder->lock_timeout = lock_timeout;

    RETVAL_OBJ_COPY(Z_OBJ_P(ZEND_THIS));
}

PHP_METHOD(TBuilder, isolation_snapshot_table_stability)
{
    ZEND_PARSE_PARAMETERS_NONE();
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
    ZEND_PARSE_PARAMETERS_NONE();
    tbuilder_update_isolation_mode(2);
}

PHP_METHOD(TBuilder, isolation_read_committed_no_record_version)
{
    ZEND_PARSE_PARAMETERS_NONE();
    tbuilder_update_isolation_mode(3);
}

PHP_METHOD(TBuilder, isolation_read_committed_read_consistency)
{
    ZEND_PARSE_PARAMETERS_NONE();
    tbuilder_update_isolation_mode(4);
}

PHP_METHOD(TBuilder, dump_state)
{
    ZEND_PARSE_PARAMETERS_NONE();
    firebird_tbuilder *b = Z_TBUILDER_P(ZEND_THIS);

    php_printf("TBuilder::dump_state\n");
    php_printf("  read_only: %d\n", b->read_only);
    php_printf("  ignore_limbo: %d\n", b->ignore_limbo);
    php_printf("  auto_commit: %d\n", b->auto_commit);
    php_printf("  no_auto_undo: %d\n", b->no_auto_undo);
    php_printf("  isolation_mode: %d\n", b->isolation_mode);
    php_printf("  lock_timeout: %d\n", b->lock_timeout);
    php_printf("  snapshot_at_number: %d\n", b->snapshot_at_number);
}

const zend_function_entry FireBird_TBuilder_methods[] = {
    PHP_ME(TBuilder, __construct, arginfo_none, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(TBuilder, read_only, arginfo_FireBird_TBuilder_flag_return_static, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, ignore_limbo, arginfo_FireBird_TBuilder_flag_return_static, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, auto_commit, arginfo_FireBird_TBuilder_flag_return_static, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, no_auto_undo, arginfo_FireBird_TBuilder_flag_return_static, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, wait, arginfo_FireBird_TBuilder_wait, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_snapshot_table_stability, arginfo_static, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_snapshot, arginfo_FireBird_TBuilder_isolation_snapshot, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_read_committed_record_version, arginfo_static, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_read_committed_no_record_version, arginfo_static, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, isolation_read_committed_read_consistency, arginfo_static, ZEND_ACC_PUBLIC)
    PHP_ME(TBuilder, dump_state, arginfo_none, ZEND_ACC_PUBLIC)

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

static zval *FireBird_TBuilder_read_property(zend_object *obj, zend_string *name, int type, void **cache_slot, zval *rv)
{
    firebird_tbuilder *b = Z_TBUILDER_O(obj);

    if (zend_string_equals_literal(name, "is_read_only")) {
        ZVAL_BOOL(rv, b->read_only);
    } else if (zend_string_equals_literal(name, "is_ignore_limbo")) {
        ZVAL_BOOL(rv, b->ignore_limbo);
    } else if (zend_string_equals_literal(name, "is_auto_commit")) {
        ZVAL_BOOL(rv, b->auto_commit);
    } else if (zend_string_equals_literal(name, "is_no_auto_undo")) {
        ZVAL_BOOL(rv, b->no_auto_undo);
    } else if (zend_string_equals_literal(name, "isolation_mode")) {
        ZVAL_LONG(rv, b->isolation_mode);
    } else if (zend_string_equals_literal(name, "lock_timeout")) {
        ZVAL_LONG(rv, b->lock_timeout);
    } else if (zend_string_equals_literal(name, "snapshot_at_number")) {
        ZVAL_LONG(rv, b->snapshot_at_number);
    } else {
        return zend_std_read_property(obj, name, type, cache_slot, rv);
    }

    return rv;
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
    FireBird_TBuilder_object_handlers.read_property = FireBird_TBuilder_read_property;
}

void tbuilder_populate_tpb(firebird_tbuilder *builder, char *tpb, unsigned short *tpb_len)
{
    char *p = tpb;

    *p++ = isc_tpb_version3;

    *p++ = builder->read_only ? isc_tpb_read : isc_tpb_write;
    if (builder->ignore_limbo) *p++ = isc_tpb_ignore_limbo;
    if (builder->auto_commit) *p++ = isc_tpb_autocommit;
    if (builder->no_auto_undo) *p++ = isc_tpb_no_auto_undo;

    if (builder->isolation_mode == 0) {
        *p++ = isc_tpb_consistency;
    } else if (builder->isolation_mode == 1) {
        *p++ = isc_tpb_concurrency;
        if (builder->snapshot_at_number) {
            *p++ = isc_tpb_at_snapshot_number;
            *p++ = sizeof(builder->snapshot_at_number);
            store_portable_integer(p, builder->snapshot_at_number, sizeof(builder->snapshot_at_number));
            p += sizeof(builder->snapshot_at_number);
        }
    } else if (builder->isolation_mode == 2) {
        *p++ = isc_tpb_read_committed;
        *p++ = isc_tpb_rec_version;
    } else if (builder->isolation_mode == 3) {
        *p++ = isc_tpb_read_committed;
        *p++ = isc_tpb_no_rec_version;
    } else if (builder->isolation_mode == 4) {
        *p++ = isc_tpb_read_committed;
        *p++ = isc_tpb_read_consistency;
    } else {
        fbp_fatal("BUG! unknown transaction isolation_mode: %d", builder->isolation_mode);
    }

    if (builder->lock_timeout == 0) {
        *p++ = isc_tpb_nowait;
    } else if (builder->lock_timeout == -1) {
        *p++ = isc_tpb_wait;
    } else if (builder->lock_timeout > 0) {
        *p++ = isc_tpb_lock_timeout;
        *p++ = sizeof(builder->lock_timeout);
        store_portable_integer(p, builder->lock_timeout, sizeof(builder->lock_timeout));
        p += sizeof(builder->lock_timeout);
    } else {
        fbp_fatal("BUG! invalid lock_timeout: %d", builder->lock_timeout);
    }

    *tpb_len = p - tpb;
}
