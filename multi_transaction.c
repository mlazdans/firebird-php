#include <ibase.h>
// #include <firebird/fb_c_api.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"

#include "fbp_multi_transaction.h"

zend_class_entry *FireBird_Multi_Transaction_ce;
static zend_object_handlers object_handlers__FireBird_Multi_Transaction_;

void FireBird_Multi_Transaction___construct(zval *MTr)
{
    firebird_multi_trans *mtr = get_firebird_multi_trans_from_zval(MTr);

    mtr->tr_handle = 0;
    mtr->tr_id = 0;
    mtr->is_prepared_2pc = 0;

    array_init(&mtr->Databases);
    array_init(&mtr->Builders);
    array_init(&mtr->Transactions);
}

PHP_METHOD(FireBird_Multi_Transaction, __construct)
{
    ZEND_PARSE_PARAMETERS_NONE();

    FireBird_Multi_Transaction___construct(ZEND_THIS);
}

void FireBird_Multi_Transaction_add_db(zval *MTr, zval *Db, zval *Builder, zval *return_value)
{
    TODO("FireBird_Multi_Transaction_add_db");
#if 0
    firebird_multi_trans *mtr = get_firebird_multi_trans_from_zval(MTr);

    Z_ADDREF_P(Db);
    add_next_index_zval(&mtr->Databases, Db);

    if (Builder && !ZVAL_IS_NULL(Builder)) {
        Z_ADDREF_P(Builder);
        add_next_index_zval(&mtr->Builders, Builder);
    } else {
        add_next_index_null(&mtr->Builders);
    }

    zval Tr;
    object_init_ex(&Tr, FireBird_Transaction_ce);

    FireBird_Transaction___construct(&Tr, Db, Builder);
    firebird_trans *tr = get_firebird_trans_from_zval(&Tr);
    tr->tr_handle = &mtr->tr_handle;

    Z_ADDREF_P(&Tr);
    add_next_index_zval(&mtr->Transactions, &Tr);

    RETVAL_OBJ(Z_OBJ(Tr));
#endif
}

PHP_METHOD(FireBird_Multi_Transaction, add_db)
{
    zval *Database, *Builder = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_OBJECT_OF_CLASS(Database, FireBird_Database_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_OBJECT_OF_CLASS(Builder, FireBird_TBuilder_ce)
    ZEND_PARSE_PARAMETERS_END();

    FireBird_Multi_Transaction_add_db(ZEND_THIS, Database, Builder, return_value);
}

void FireBird_Multi_Transaction_start(zval *MTr, zval *return_value)
{
    firebird_multi_trans *mtr = get_firebird_multi_trans_from_zval(MTr);

    if (fbp_multi_transaction_start(mtr)) {
        update_err_props(FBG(status), FireBird_Multi_Transaction_ce, MTr);
        RETURN_FALSE;
    }

    zend_update_property_long(FireBird_Multi_Transaction_ce, Z_OBJ_P(MTr), "id", 2, (zend_long)mtr->tr_id);

    RETURN_TRUE;
}

PHP_METHOD(FireBird_Multi_Transaction, start) {
    ZEND_PARSE_PARAMETERS_NONE();

    FireBird_Multi_Transaction_start(ZEND_THIS, return_value);
}

static void _FireBird_Multi_Transaction_finalize(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
    TODO("_FireBird_Multi_Transaction_finalize");
#if 0
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_multi_trans *mtr = get_firebird_multi_trans_from_zval(ZEND_THIS);

    if (fbp_transaction_finalize(&mtr->tr_handle, mode)) {
        update_err_props(FBG(status), FireBird_Multi_Transaction_ce, ZEND_THIS);
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
#endif
}

PHP_METHOD(FireBird_Multi_Transaction, commit) {
    _FireBird_Multi_Transaction_finalize(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_TR_COMMIT);
}

PHP_METHOD(FireBird_Multi_Transaction, commit_ret) {
    _FireBird_Multi_Transaction_finalize(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_TR_COMMIT | FBP_TR_RETAIN);
}

PHP_METHOD(FireBird_Multi_Transaction, rollback) {
    _FireBird_Multi_Transaction_finalize(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_TR_ROLLBACK);
}

PHP_METHOD(FireBird_Multi_Transaction, rollback_ret) {
    _FireBird_Multi_Transaction_finalize(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_TR_ROLLBACK | FBP_TR_RETAIN);
}

PHP_METHOD(FireBird_Multi_Transaction, prepare_2pc)
{
    char *description = NULL;
    size_t description_len = 0;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(description, description_len)
    ZEND_PARSE_PARAMETERS_END();

    firebird_multi_trans *mtr = get_firebird_multi_trans_from_zval(ZEND_THIS);

    // TODO: add description to standard single db 2pc
    // If description or if multi db transaction 2pc, then FB puts entry in RDB$TRANSACTIONS.
    // 1 RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.LIMBO
	// 2 RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.COMMITTED
    // 3 RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.ROLLED_BACK
    if (isc_prepare_transaction2(FBG(status), &mtr->tr_handle, description_len, description)) {
        update_err_props(FBG(status), FireBird_Multi_Transaction_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    mtr->is_prepared_2pc = 1;

    RETURN_TRUE;
}

// const zend_function_entry FireBird_Multi_Transaction_methods[] = {
//     PHP_ME(Multi_Transaction, __construct, arginfo_none, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
//     PHP_ME(Multi_Transaction, add_db, arginfo_FireBird_Multi_Transaction_add_db, ZEND_ACC_PUBLIC)
//     PHP_ME(Multi_Transaction, start, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Multi_Transaction, commit, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Multi_Transaction, commit_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Multi_Transaction, rollback, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Multi_Transaction, rollback_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
//     PHP_ME(Multi_Transaction, prepare_2pc, arginfo_FireBird_Multi_Transaction_prepare_2pc, ZEND_ACC_PUBLIC)
//     PHP_FE_END
// };

static zend_object *new__FireBird_Multi_Transaction_(zend_class_entry *ce)
{
    firebird_multi_trans *tr = zend_object_alloc(sizeof(firebird_multi_trans), ce);

    zend_object_std_init(&tr->std, ce);
    object_properties_init(&tr->std, ce);

    return &tr->std;
}

static void free_FireBird_Multi_Transaction(zend_object *obj)
{
    TODO("free_FireBird_Multi_Transaction");
#if 0
    firebird_multi_trans *mtr = get_firebird_multi_trans_from_obj(obj);

    FBDEBUG("free_FireBird_Multi_Transaction: %p: %zu", &mtr->tr_handle, mtr->tr_handle);

    // Restore tr_handle pointer to self
    zend_array *tr_arr = Z_ARR(mtr->Transactions);
    for (int i = 0; i < tr_arr->nNumOfElements; i++) {
        zval *Tr = zend_hash_index_find(tr_arr, i);
        firebird_trans *tr = get_firebird_trans_from_zval(Tr);
        tr->tr_handle = &tr->real_tr_handle;
    }

    zval_ptr_dtor(&mtr->Transactions);
    zval_ptr_dtor(&mtr->Builders);
    zval_ptr_dtor(&mtr->Databases);

    // MAYBE: not to close automatically in some strict mode or smth
    if(mtr->tr_handle && !mtr->is_prepared_2pc) {
        if(isc_rollback_transaction(FBG(status), &mtr->tr_handle)) {
            fbp_status_error(FBG(status));
        }
    }

    zend_object_std_dtor(&mtr->std);
#endif
}

// void register_FireBird_Multi_Transaction_ce()
// {
//     zend_class_entry tmp_ce;
//     INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Multi_Transaction", FireBird_Multi_Transaction_methods);
//     FireBird_Multi_Transaction_ce = zend_register_internal_class(&tmp_ce);

//     // DECLARE_PROP_OBJ(FireBird_Multi_Transaction_ce, database, FireBird\\Database, ZEND_ACC_PROTECTED_SET);
//     // DECLARE_PROP_OBJ(FireBird_Multi_Transaction_ce, builder, FireBird\\TBuilder, ZEND_ACC_PROTECTED_SET);
//     DECLARE_PROP_LONG(FireBird_Multi_Transaction_ce, id, ZEND_ACC_PROTECTED_SET);
//     DECLARE_ERR_PROPS(FireBird_Multi_Transaction_ce);

//     zend_class_implements(FireBird_Multi_Transaction_ce, 1, FireBird_IError_ce);

//     FireBird_Multi_Transaction_ce->create_object = new__FireBird_Multi_Transaction_;
//     FireBird_Multi_Transaction_ce->default_object_handlers = &object_handlers__FireBird_Multi_Transaction_;

//     memcpy(&object_handlers__FireBird_Multi_Transaction_, &std_object_handlers, sizeof(zend_object_handlers));

//     object_handlers__FireBird_Multi_Transaction_.offset = XtOffsetOf(firebird_multi_trans, std);
//     object_handlers__FireBird_Multi_Transaction_.free_obj = free_FireBird_Multi_Transaction;
// }
