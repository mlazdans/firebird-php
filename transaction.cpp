#include "firebird_php.hpp"
#include "firebird_utils.h"

extern "C" {

#include <ibase.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"

fbp_object_accessor(firebird_trans);
fbp_object_accessor(firebird_tbuilder);
static zend_object_handlers FireBird_Transaction_object_handlers;

int FireBird_Transaction___construct(zval *self, zval *database)
{
    PROP_SET(FireBird_Transaction_ce, self, "database", database);

    firebird_trans *tr = get_firebird_trans_from_zval(self);
    firebird_db *db = get_firebird_db_from_zval(database);

    size_t trh;
    const firebird_trans_info *info;

    if (fbu_transaction_init(db->dbh, &trh)) {
        return FAILURE;
    }

    if (fbu_transaction_get_info(db->dbh, trh, &info)) {
        return FAILURE;
    }

    tr->trh = trh;
    tr->dbh = db->dbh;
    tr->info = info;

    return SUCCESS;
}

PHP_METHOD(FireBird_Transaction, __construct)
{
    zval *database;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(database, FireBird_Database_ce)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Transaction___construct(ZEND_THIS, database)) {
        RETURN_THROWS();
    }
}

int FireBird_Transaction_start(zval *self, zval *builder)
{
    firebird_trans *tr = get_firebird_trans_from_zval(self);
    firebird_tbuilder *tb = get_firebird_tbuilder_from_zval(builder);

    FBDEBUG("FireBird_Transaction_start(tr=%p, tr->trh=%lu, tb=%p)", tr, tr->trh, tb);

    return fbu_transaction_start(tr->dbh, tr->trh, tb);
}

PHP_METHOD(FireBird_Transaction, start)
{
    zval *builder = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_OBJECT_OF_CLASS(builder, FireBird_TBuilder_ce)
    ZEND_PARSE_PARAMETERS_END();

    FBDEBUG("%s(builder=%p)", __func__, builder);

    if (FireBird_Transaction_start(ZEND_THIS, builder)) {
        RETURN_THROWS();
    }
}

static void FireBird_Transaction_finalize(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);

    FBDEBUG("%s(tr=%p, mode=%d)", __func__, tr, mode);

    if (fbu_transaction_finalize(tr->dbh, tr->trh, mode)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Transaction, commit) {
    FireBird_Transaction_finalize(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_TR_COMMIT);
}

PHP_METHOD(FireBird_Transaction, commit_ret) {
    FireBird_Transaction_finalize(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_TR_COMMIT | FBP_TR_RETAIN);
}

PHP_METHOD(FireBird_Transaction, rollback) {
    FireBird_Transaction_finalize(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_TR_ROLLBACK);
}

PHP_METHOD(FireBird_Transaction, rollback_ret) {
    FireBird_Transaction_finalize(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_TR_ROLLBACK | FBP_TR_RETAIN);
}

PHP_METHOD(FireBird_Transaction, prepare)
{
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Statement_ce);
    if (
        FireBird_Statement___construct(return_value, ZEND_THIS) ||
        FireBird_Statement_prepare(return_value, sql)
    ) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Transaction, query)
{
    zval *bind_args;
    uint32_t num_bind_args;
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(sql)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Statement_ce);
    if (
        FireBird_Statement___construct(return_value, ZEND_THIS) ||
        FireBird_Statement_prepare(return_value, sql) ||
        FireBird_Statement_execute(return_value, bind_args, num_bind_args)
    ) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Transaction, execute)
{
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);

    if (fbu_transaction_execute(tr->dbh, tr->trh, ZSTR_LEN(sql), ZSTR_VAL(sql))) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Transaction, open_blob)
{
    zval *Blob_Id;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Blob_Id, FireBird_Blob_Id_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Blob_ce);
    if (FireBird_Blob___construct(return_value, ZEND_THIS) || FireBird_Blob_open(return_value, Blob_Id)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Transaction, create_blob)
{
    ZEND_PARSE_PARAMETERS_NONE();

    object_init_ex(return_value, FireBird_Blob_ce);
    if (FireBird_Blob___construct(return_value, ZEND_THIS) || FireBird_Blob_create(return_value)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Transaction, prepare_2pc)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);

    if (fbu_transaction_prepare(tr->dbh, tr->trh)) {
        RETURN_THROWS();
    }

    tr->is_prepared_2pc = 1;
}

static zend_object *FireBird_Transaction_create_object(zend_class_entry *ce)
{
    firebird_trans *tr = (firebird_trans *)zend_object_alloc(sizeof(firebird_trans), ce);

    FBDEBUG("+%s(tr=%p, tr->trh=%lu)", __func__, tr, tr->trh);

    zend_object_std_init(&tr->std, ce);
    object_properties_init(&tr->std, ce);

    return &tr->std;
}

static void FireBird_Transaction_free_obj(zend_object *obj)
{
    firebird_trans *tr = get_firebird_trans_from_obj(obj);

    FBDEBUG("~%s(tr=%p, tr->trh=%zu)", __func__, tr, tr->trh);

    if (fbu_is_valid_trh(tr->dbh, tr->trh) && !tr->is_prepared_2pc) {
        if (fbu_transaction_free(tr->dbh, tr->trh)) {
            // throws
        }
        tr->trh = 0;
    }

    zend_object_std_dtor(&tr->std);
}

static zval* FireBird_Transaction_read_property(zend_object *obj, zend_string *name, int type,
    void **cache_slot, zval *rv)
{
    firebird_trans *tr = get_firebird_trans_from_obj(obj);

    if (zend_string_equals_literal(name, "id")) {
        ZVAL_LONG(rv, tr->info->id);
        return rv;
    }

    return zend_std_read_property(obj, name, type, cache_slot, rv);
}

void register_FireBird_Transaction_object_handlers()
{
    FireBird_Transaction_ce->default_object_handlers = &FireBird_Transaction_object_handlers;
    FireBird_Transaction_ce->create_object = FireBird_Transaction_create_object;

    memcpy(&FireBird_Transaction_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Transaction_object_handlers.offset = XtOffsetOf(firebird_trans, std);
    FireBird_Transaction_object_handlers.free_obj = FireBird_Transaction_free_obj;
    FireBird_Transaction_object_handlers.read_property = FireBird_Transaction_read_property;
}


}