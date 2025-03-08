#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Transaction_ce;
static zend_object_handlers FireBird_Transaction_object_handlers;

PHP_METHOD(Transaction, __construct) {
    // zend_string *database = NULL, *username = NULL, *password = NULL, *charset = NULL, *role = NULL;
    zend_long trans_args = 0, lock_timeout = 0;
    zval *connection_obj;
    zend_object *obj;

    bool trans_args_is_null = 1, lock_timeout_is_null = 1;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_OBJECT_OF_CLASS(connection_obj, FireBird_Connection_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(trans_args, trans_args_is_null)
        Z_PARAM_LONG_OR_NULL(lock_timeout, lock_timeout_is_null)
    ZEND_PARSE_PARAMETERS_END();

    // obj = Z_OBJ_P(connection_obj);
    // php_printf("obj->ce->name = %s\n", obj->ce->name->val);

    if(!lock_timeout_is_null) zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "trans_args", sizeof("trans_args") - 1, trans_args);
    if(!lock_timeout_is_null) zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "lock_timeout", sizeof("lock_timeout") - 1, lock_timeout);
}

const zend_function_entry FireBird_Transaction_methods[] = {
    PHP_ME(Transaction, __construct, arginfo_FireBird_Transaction_construct, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Transaction_create(zend_class_entry *ce)
{
    php_printf("firebird_transaction_create\n");
    firebird_trans *conn = zend_object_alloc(sizeof(firebird_trans), ce);

    zend_object_std_init(&conn->std, ce);
    object_properties_init(&conn->std, ce);

    conn->handle = 0;

    return &conn->std;
}

static void FireBird_Transaction_free_obj(zend_object *obj)
{
    php_printf("FireBird_Transaction_free_obj\n");
    firebird_trans *conn = Z_TRANSACTION_O(obj);
    zend_object_std_dtor(&conn->std);
}

void register_FireBird_Transaction_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Transaction", FireBird_Transaction_methods);
    FireBird_Transaction_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Transaction_ce, connection, FireBird\\Connection, ZEND_ACC_PROTECTED);
    DECLARE_PROP_INT(FireBird_Transaction_ce, trans_args, ZEND_ACC_PROTECTED);
    DECLARE_PROP_INT(FireBird_Transaction_ce, lock_timeout, ZEND_ACC_PROTECTED);
    ADD_ERR_PROPS(FireBird_Transaction_ce);

    FireBird_Transaction_ce->create_object = FireBird_Transaction_create;
    FireBird_Transaction_ce->default_object_handlers = &FireBird_Transaction_object_handlers;

    memcpy(&FireBird_Transaction_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Transaction_object_handlers.offset = XtOffsetOf(firebird_trans, std);
    FireBird_Transaction_object_handlers.free_obj = FireBird_Transaction_free_obj;
}

void _php_ibase_populate_trans(zend_long trans_argl, zend_long trans_timeout, char *last_tpb, unsigned short *len)
{
    unsigned short tpb_len = 0;

    *len = tpb_len;

    if (trans_argl == 0) {
        return;
    }

    last_tpb[tpb_len++] = isc_tpb_version3;

    /* access mode */
    if (PHP_FIREBIRD_READ == (trans_argl & PHP_FIREBIRD_READ)) {
        last_tpb[tpb_len++] = isc_tpb_read;
    } else if (PHP_FIREBIRD_WRITE == (trans_argl & PHP_FIREBIRD_WRITE)) {
        last_tpb[tpb_len++] = isc_tpb_write;
    }

    /* isolation level */
    if (PHP_FIREBIRD_COMMITTED == (trans_argl & PHP_FIREBIRD_COMMITTED)) {
        last_tpb[tpb_len++] = isc_tpb_read_committed;
        if (PHP_FIREBIRD_REC_VERSION == (trans_argl & PHP_FIREBIRD_REC_VERSION)) {
            last_tpb[tpb_len++] = isc_tpb_rec_version;
        } else if (PHP_FIREBIRD_REC_NO_VERSION == (trans_argl & PHP_FIREBIRD_REC_NO_VERSION)) {
            last_tpb[tpb_len++] = isc_tpb_no_rec_version;
        }
    } else if (PHP_FIREBIRD_CONSISTENCY == (trans_argl & PHP_FIREBIRD_CONSISTENCY)) {
        last_tpb[tpb_len++] = isc_tpb_consistency;
    } else if (PHP_FIREBIRD_CONCURRENCY == (trans_argl & PHP_FIREBIRD_CONCURRENCY)) {
        last_tpb[tpb_len++] = isc_tpb_concurrency;
    }

    /* lock resolution */
    if (PHP_FIREBIRD_NOWAIT == (trans_argl & PHP_FIREBIRD_NOWAIT)) {
        last_tpb[tpb_len++] = isc_tpb_nowait;
    } else if (PHP_FIREBIRD_WAIT == (trans_argl & PHP_FIREBIRD_WAIT)) {
        last_tpb[tpb_len++] = isc_tpb_wait;
        if (PHP_FIREBIRD_LOCK_TIMEOUT == (trans_argl & PHP_FIREBIRD_LOCK_TIMEOUT)) {
            if (trans_timeout <= 0 || trans_timeout > 0x7FFF) {
                php_error_docref(NULL, E_WARNING, "Invalid lock timeout parameter");
            } else {
                last_tpb[tpb_len++] = isc_tpb_lock_timeout;
                last_tpb[tpb_len++] = sizeof(ISC_SHORT);
                last_tpb[tpb_len] = (ISC_SHORT)trans_timeout;
                tpb_len += sizeof(ISC_SHORT);
            }
        }
    }

    *len = tpb_len;
}
