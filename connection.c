#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Connection_ce;

PHP_METHOD(Connection, new_transaction) {
    zend_long trans_args = 0, lock_timeout = 0;

    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(trans_args)
        Z_PARAM_LONG(lock_timeout)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Transaction_ce);

    transaction_ctor(return_value, ZEND_THIS, trans_args, lock_timeout);
}

void connection_ctor(zval *conn_o, zval *database)
{
    zend_update_property(FireBird_Connection_ce, O_SET(conn_o, database));
}

PHP_METHOD(Connection, __construct)
{
}

PHP_METHOD(Connection, disconnect) {
    ZEND_PARSE_PARAMETERS_NONE();

    zval rv, *database;

    FBDEBUG("Connection::disconnect()");

    database = zend_read_property(FireBird_Connection_ce, THIS_GET(database), 0, &rv);
    firebird_db *db = Z_DB_P(database);

    if(db->db_handle) {
        ISC_STATUS_ARRAY status;
        FBDEBUG("Closing handle: %d", db->db_handle);
        if(isc_detach_database(status, &db->db_handle)){
            update_err_props(status, FireBird_Connection_ce, ZEND_THIS);
        } else {
            db->db_handle = 0;
            RETURN_TRUE;
        }
    }

    RETURN_FALSE;
}

PHP_METHOD(Connection, reconnect_transaction)
{
    ISC_STATUS_ARRAY status;
    zend_long id;
    zval rv, *database;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(id)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Transaction_ce);
    transaction_ctor(return_value, ZEND_THIS, 0, 0);

    firebird_trans *tr = Z_TRANSACTION_P(return_value);

    FBDEBUG("Connection, reconnect_transaction: %d", id);
    if (
        isc_reconnect_transaction(status, tr->db_handle, &tr->tr_handle, sizeof(id), (const ISC_SCHAR *)&id) ||
        transaction_get_info(status, tr)) {
            update_err_props(status, FireBird_Connection_ce, ZEND_THIS);
            zval_ptr_dtor(return_value);
            RETURN_FALSE;
    }
}

const zend_function_entry FireBird_Connection_methods[] = {
    PHP_ME(Connection, __construct, arginfo_none, ZEND_ACC_PRIVATE)
    PHP_ME(Connection, disconnect, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Connection, new_transaction, arginfo_FireBird_Connection_new_transaction, ZEND_ACC_PUBLIC)
    PHP_ME(Connection, reconnect_transaction, arginfo_FireBird_Connection_reconnect_transaction, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void register_FireBird_Connection_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Connection", FireBird_Connection_methods);
    FireBird_Connection_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Connection_ce, database, FireBird\\Database, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Connection_ce);

    {
        zend_type_list *list = malloc(ZEND_TYPE_LIST_SIZE(2)); // keep malloc here
        list->num_types = 2;
        list->types[0] = (zend_type) ZEND_TYPE_INIT_CLASS(zend_string_init("FireBird\\Create_Args", sizeof("FireBird\\Create_Args") - 1, 1), 0, 0);
        list->types[1] = (zend_type) ZEND_TYPE_INIT_CLASS(zend_string_init("FireBird\\Connect_Args", sizeof("FireBird\\Connect_Args") - 1, 1), 0, 0);
        zend_type prop_type = ZEND_TYPE_INIT_UNION(list, 0);
        zend_string *prop_name = zend_string_init("args", sizeof("args") - 1, 1);
        zval def;
        ZVAL_UNDEF(&def);

        zend_declare_typed_property(FireBird_Connection_ce, prop_name, &def, ZEND_ACC_PUBLIC, NULL, prop_type);
        zend_string_release(prop_name);
    }

    zend_class_implements(FireBird_Connection_ce, 1, FireBird_IError_ce);
}
