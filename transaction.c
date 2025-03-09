#include <ibase.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Transaction_ce;
static zend_object_handlers FireBird_Transaction_object_handlers;

#define ROLLBACK    0
#define COMMIT      1
#define RETAIN      2

PHP_METHOD(Transaction, __construct) {
    zend_long trans_args = 0, lock_timeout = 0;
    zval *connection;
    bool trans_args_is_null = 1, lock_timeout_is_null = 1;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_OBJECT_OF_CLASS(connection, FireBird_Connection_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(trans_args, trans_args_is_null)
        Z_PARAM_LONG_OR_NULL(lock_timeout, lock_timeout_is_null)
    ZEND_PARSE_PARAMETERS_END();

    zend_update_property(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "connection", sizeof("connection") - 1, connection);

    if(!trans_args_is_null) zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "trans_args", sizeof("trans_args") - 1, trans_args);
    if(!lock_timeout_is_null) zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "lock_timeout", sizeof("lock_timeout") - 1, lock_timeout);

    firebird_connection *conn = Z_CONNECTION_P(connection);
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);
    tr->db_handle = conn->handle;
}

PHP_METHOD(Transaction, start) {
    ZEND_PARSE_PARAMETERS_NONE();

    zval rv, *val;
    zend_long trans_args = 0, lock_timeout = 0;
    ISC_STATUS_ARRAY status;
    ISC_STATUS result;
    char tpb[TPB_MAX_SIZE];
    unsigned short tpb_len = 0;
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);

    val = zend_read_property(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "trans_args", sizeof("trans_args") - 1, 1, &rv);
    trans_args = Z_LVAL_P(val);

    val = zend_read_property(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "lock_timeout", sizeof("lock_timeout") - 1, 1, &rv);
    lock_timeout = Z_LVAL_P(val);

    php_printf("trans_args=%d, lock_timeout=%d\n", trans_args, lock_timeout);

    populate_trans(trans_args, lock_timeout, tpb, &tpb_len);
    if(isc_start_transaction(status, &tr->tr_handle, 1, &tr->db_handle, tpb_len, tpb)) {
        update_err_props(status, FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

bool process_trans(zend_object *obj, int commit) {
    zval rv, *val;
    ISC_STATUS result;
    ISC_STATUS_ARRAY status;
    firebird_trans *tr = Z_TRANSACTION_O(obj);

    switch (commit) {
        default: /* == case ROLLBACK: */
            result = isc_rollback_transaction(status, &tr->tr_handle);
            break;
        case COMMIT:
            result = isc_commit_transaction(status, &tr->tr_handle);
            break;
        case (ROLLBACK | RETAIN):
            result = isc_rollback_retaining(status, &tr->tr_handle);
            break;
        case (COMMIT | RETAIN):
            result = isc_commit_retaining(status, &tr->tr_handle);
            break;
    }

    if (result) {
        update_err_props(status, FireBird_Transaction_ce, obj);
        return false;
    } else {
        return true;
    }
}

PHP_METHOD(Transaction, commit) {
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_BOOL(process_trans(Z_OBJ_P(ZEND_THIS), COMMIT));
}

PHP_METHOD(Transaction, commit_ret) {
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_BOOL(process_trans(Z_OBJ_P(ZEND_THIS), COMMIT | RETAIN));
}

PHP_METHOD(Transaction, rollback) {
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_BOOL(process_trans(Z_OBJ_P(ZEND_THIS), ROLLBACK));
}

PHP_METHOD(Transaction, rollback_ret) {
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_BOOL(process_trans(Z_OBJ_P(ZEND_THIS), ROLLBACK | RETAIN));
}

const zend_function_entry FireBird_Transaction_methods[] = {
    PHP_ME(Transaction, __construct, arginfo_FireBird_Transaction_construct, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, start, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, commit, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, commit_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, rollback, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, rollback_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)

    PHP_FE_END
};

static zend_object *FireBird_Transaction_create(zend_class_entry *ce)
{
    firebird_trans *tr = zend_object_alloc(sizeof(firebird_trans), ce);

    zend_object_std_init(&tr->std, ce);
    object_properties_init(&tr->std, ce);

    return &tr->std;
}

static void FireBird_Transaction_free_obj(zend_object *obj)
{
    php_printf("FireBird_Transaction_free_obj\n");
    firebird_trans *tr = Z_TRANSACTION_O(obj);

    if(tr->tr_handle) {
        ISC_STATUS_ARRAY status;
        if(isc_rollback_transaction(status, &tr->tr_handle)) {
            // TODO: report errors?
        }
        tr->tr_handle = 0;
    }
    zend_object_std_dtor(&tr->std);
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

void populate_trans(zend_long trans_argl, zend_long trans_timeout, char *last_tpb, unsigned short *len)
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
            last_tpb[tpb_len++] = isc_tpb_lock_timeout;
            last_tpb[tpb_len++] = sizeof(ISC_SHORT);
            last_tpb[tpb_len] = (ISC_SHORT)trans_timeout;
            tpb_len += sizeof(ISC_SHORT);
        }
    }

    *len = tpb_len;
}
