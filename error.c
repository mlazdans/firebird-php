#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"

#include "php_firebird_includes.h"
#include "pdo_firebird_utils.h"

zend_class_entry *FireBird_IError_ce;

void register_FireBird_IError_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "IError", NULL);
    FireBird_IError_ce = zend_register_internal_interface(&tmp_ce);
}
