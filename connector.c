#include "php.h"

#include "database.h"
#include "fbp_database.h"
#include "firebird_utils.h"

PHP_METHOD(FireBird_Connector, connect)
{
    zval *Connect_Args;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Connect_Args, FireBird_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Database_ce);
    firebird_db *db = get_firebird_db_from_zval(return_value);

    if (fbu_attach_database(FBG(status), db, Connect_Args, FireBird_Connect_Args_ce)) {
        update_err_props(FBG(status), FireBird_Connector_ce, Conn);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }

    OBJ_SET(FireBird_Database_ce, return_value, "args", Connect_Args);
}

// TODO: get rid off
static int FireBird_Connector_create(zval *Conn, zval *Db, zval *Create_Args)
{
    firebird_db *db = get_firebird_db_from_zval(Db);

    if (fbp_database_create(db, Create_Args)) {
        update_err_props(FBG(status), FireBird_Connector_ce, Conn);
        return FAILURE;
    }

    OBJ_SET(FireBird_Database_ce, Db, "args", Create_Args);

    return SUCCESS;
}

PHP_METHOD(FireBird_Connector, create)
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
