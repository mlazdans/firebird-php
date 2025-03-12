#include <ibase.h>
#include "php.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Connect_Args_ce;

void register_FireBird_Connect_Args_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Connect_Args", NULL);
    FireBird_Connect_Args_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_STRING(FireBird_Connect_Args_ce, database, ZEND_ACC_PUBLIC);
    DECLARE_PROP_STRING(FireBird_Connect_Args_ce, username, ZEND_ACC_PUBLIC);
    DECLARE_PROP_STRING(FireBird_Connect_Args_ce, password, ZEND_ACC_PUBLIC);
    DECLARE_PROP_STRING(FireBird_Connect_Args_ce, role, ZEND_ACC_PUBLIC);
    DECLARE_PROP_STRING(FireBird_Connect_Args_ce, charset, ZEND_ACC_PUBLIC);
    DECLARE_PROP_LONG(FireBird_Connect_Args_ce, buffers, ZEND_ACC_PUBLIC);
}
