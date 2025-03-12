#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"

#include "php_firebird_includes.h"
#include "pdo_firebird_utils.h"

zend_class_entry *FireBird_IError_ce;
static zend_object_handlers FireBird_IError_object_handlers;

const zend_function_entry FireBird_IError_methods[] = {
    PHP_FE_END
};

void register_FireBird_IError_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "IError", FireBird_IError_methods);
    FireBird_IError_ce = zend_register_internal_interface(&tmp_ce);

    DECLARE_ERR_PROPS(FireBird_IError_ce);
}
