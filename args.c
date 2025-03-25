#include <ibase.h>
#include "php.h"
#include "php_firebird_includes.h"
#include "fbp_database.h"

zend_class_entry *FireBird_Service_Connect_Args_ce;

void register_FireBird_Service_Connect_Args_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Service_Connect_Args", NULL);
    FireBird_Service_Connect_Args_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_STRING(FireBird_Service_Connect_Args_ce, service_name, ZEND_ACC_PUBLIC);
    declare_props_zmap(FireBird_Service_Connect_Args_ce, &service_connect_zmap);
}
