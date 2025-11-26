#include <ibase.h>
#include "php.h"

// zend_class_entry *FireBird_Var_Info_ce;

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

void FireBird_Var_Info_from_var(zval *return_value, XSQLVAR *var)
{
    object_init_ex(return_value, FireBird_Var_Info_ce);
    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "name", sizeof("name") - 1, var->sqlname);
    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "alias", sizeof("alias") - 1, var->aliasname);
    zend_update_property_string(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "relation", sizeof("relation") - 1, var->relname);
    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "byte_length", sizeof("byte_length") - 1, var->sqllen);
    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "type", sizeof("type") - 1, var->sqltype & ~1);
    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "sub_type", sizeof("sub_type") - 1, var->sqlsubtype);
    zend_update_property_long(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "scale", sizeof("scale") - 1, var->sqlscale);
    zend_update_property_bool(FireBird_Var_Info_ce, Z_OBJ_P(return_value), "nullable", sizeof("nullable") - 1, var->sqltype & 1);
}
