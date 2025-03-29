#include "php.h"

#include "database.h"
#include "fbp_database.h"

zend_class_entry *FireBird_Connector_ce;

int FireBird_Connector_connect(zval *Conn, zval *Db, zval *Connect_Args)
{
    firebird_db *db = get_firebird_db_from_zval(Db);

    if (fbp_database_connect(db, Connect_Args)) {
        update_err_props(FBG(status), FireBird_Connector_ce, Conn);
        return FAILURE;
    }

    OBJ_SET(FireBird_Connector_ce, Db, "args", Connect_Args);

    return SUCCESS;
}

PHP_METHOD(Connector, connect)
{
    zval *Connect_Args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Connect_Args, FireBird_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Database_ce);
    if (FireBird_Connector_connect(ZEND_THIS, return_value, Connect_Args)) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

int FireBird_Connector_create(zval *Conn, zval *Db, zval *Create_Args)
{
    firebird_db *db = get_firebird_db_from_zval(Db);

    if (fbp_database_create(db, Create_Args)) {
        update_err_props(FBG(status), FireBird_Connector_ce, Conn);
        return FAILURE;
    }

    OBJ_SET(FireBird_Database_ce, Db, "args", Create_Args);

    return SUCCESS;
}

PHP_METHOD(Connector, create)
{
    zval *Create_Args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Create_Args, FireBird_Create_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Database_ce);
    if (FireBird_Connector_create(ZEND_THIS, return_value, Create_Args)) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

const zend_function_entry FireBird_Connector_methods[] = {
    PHP_ME(Connector, connect, arginfo_FireBird_Connector_connect, ZEND_ACC_PUBLIC)
    PHP_ME(Connector, create, arginfo_FireBird_Connector_create, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void register_FireBird_Connector_ce()
{
    zend_class_entry tmp_ce;

    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Connector", FireBird_Connector_methods);
    FireBird_Connector_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_ERR_PROPS(FireBird_Connector_ce);

    zend_class_implements(FireBird_Connector_ce, 1, FireBird_IError_ce);
}
