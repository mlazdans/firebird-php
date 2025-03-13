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
    declare_props_zmap(FireBird_Create_Args_ce, &database_create_zmap);
}
