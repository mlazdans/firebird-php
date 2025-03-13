#include <ibase.h>
#include "php.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Create_Args_ce;

void register_FireBird_Create_Args_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Create_Args", NULL);
    FireBird_Create_Args_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_STRING(FireBird_Create_Args_ce, database, ZEND_ACC_PUBLIC);
    DECLARE_PROP_STRING(FireBird_Create_Args_ce, username, ZEND_ACC_PUBLIC);
    DECLARE_PROP_STRING(FireBird_Create_Args_ce, password, ZEND_ACC_PUBLIC);
    DECLARE_PROP_STRING(FireBird_Create_Args_ce, charset, ZEND_ACC_PUBLIC);
    DECLARE_PROP_LONG(FireBird_Create_Args_ce, sweep_interval, ZEND_ACC_PUBLIC);
    DECLARE_PROP_LONG(FireBird_Create_Args_ce, buffers, ZEND_ACC_PUBLIC);
    DECLARE_PROP_LONG(FireBird_Create_Args_ce, page_size, ZEND_ACC_PUBLIC);
    DECLARE_PROP_BOOL(FireBird_Create_Args_ce, force_write, ZEND_ACC_PUBLIC);
}
