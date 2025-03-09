#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Statement_ce;
static zend_object_handlers FireBird_Statement_object_handlers;

static void free_stmt(firebird_stmt *s);

PHP_METHOD(Statement, fetch_row)
{
    firebird_stmt *q = Z_STMT_P(ZEND_THIS);
}

const zend_function_entry FireBird_Statement_methods[] = {
    PHP_ME(Statement, fetch_row, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Statement_create(zend_class_entry *ce)
{
    php_printf("FireBird_Statement_create()\n");
    firebird_stmt *s = zend_object_alloc(sizeof(firebird_stmt), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Statement_free_obj(zend_object *obj)
{
    php_printf("FireBird_Statement_free_obj\n");
    firebird_stmt *s = Z_STMT_O(obj);

    free_stmt(s);

    zend_object_std_dtor(&s->std);
}

void register_FireBird_Statement_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Statement", FireBird_Statement_methods);
    FireBird_Statement_ce = zend_register_internal_class(&tmp_ce);

    ADD_ERR_PROPS(FireBird_Statement_ce);

    FireBird_Statement_ce->create_object = FireBird_Statement_create;
    FireBird_Statement_ce->default_object_handlers = &FireBird_Statement_object_handlers;

    memcpy(&FireBird_Statement_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Statement_object_handlers.offset = XtOffsetOf(firebird_stmt, std);
    FireBird_Statement_object_handlers.free_obj = FireBird_Statement_free_obj;
}

static void free_stmt(firebird_stmt *s)
{
    if (s->in_sqlda) {
        efree(s->in_sqlda);
    }
    if (s->out_sqlda) {
        efree(s->out_sqlda);
    }
    if (s->in_array) {
        efree(s->in_array);
    }
    if (s->out_array) {
        efree(s->out_array);
    }
}
