#include <ibase.h>
#include "php.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Var_Info_ce;

void register_FireBird_Var_Info_ce()
{
    zend_class_entry tmp_ce;

    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Var_Info", NULL);
    FireBird_Var_Info_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_STRING(FireBird_Var_Info_ce, name, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_STRING(FireBird_Var_Info_ce, alias, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_STRING(FireBird_Var_Info_ce, relation, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Var_Info_ce, byte_length, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Var_Info_ce, type, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Var_Info_ce, sub_type, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Var_Info_ce, scale, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_BOOL(FireBird_Var_Info_ce, nullable, ZEND_ACC_PROTECTED_SET);
}
