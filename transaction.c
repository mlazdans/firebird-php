#include <ibase.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"
#include "firebird_utils.h"

#include "blob.h"
#include "database.h"
#include "transaction.h"
#include "statement.h"
#include "fbp_statement.h"
#include "fbp_blob.h"

static zend_object_handlers FireBird_Transaction_object_handlers;
fbp_object_accessor(firebird_trans);
fbp_object_accessor(firebird_tbuilder);

void FireBird_Transaction___construct(zval *self, zval *database)
{
    PROP_SET(FireBird_Transaction_ce, self, "database", database);

    firebird_trans *tr = get_firebird_trans_from_zval(self);
    firebird_db *db = get_firebird_db_from_zval(database);

    fbu_transaction_init(db, tr);

    FBDEBUG("FireBird_Transaction___construct finished(db=%p, tr=%p, tr->trptr=%p)", db, tr, tr->trptr);
}

PHP_METHOD(FireBird_Transaction, __construct)
{
    zval *database;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(database, FireBird_Database_ce)
    ZEND_PARSE_PARAMETERS_END();

    FireBird_Transaction___construct(ZEND_THIS, database);
}

int FireBird_Transaction_start(zval *self, zval *builder)
{
    firebird_trans *tr = get_firebird_trans_from_zval(self);
    firebird_tbuilder *tb = get_firebird_tbuilder_from_zval(builder);

    FBDEBUG("######## FireBird_Transaction_start(tr=%p, tb=%p", tr, tb);

    return fbu_transaction_start(tr, tb);
}

PHP_METHOD(FireBird_Transaction, start)
{
    zval *builder = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_OBJECT_OF_CLASS(builder, FireBird_TBuilder_ce)
    ZEND_PARSE_PARAMETERS_END();

    FBDEBUG("!!!!!!!!! FireBird_Transaction, builder:%p", builder);

    if (FireBird_Transaction_start(ZEND_THIS, builder)) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

static void FireBird_Transaction_finalize(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);

    FBDEBUG("FireBird_Transaction_finalize: %p, mode: %d", tr, mode);

    if (fbu_transaction_finalize(tr, mode)) {
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
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

// int FireBird_Transaction_prepare(zval *self, const ISC_SCHAR* sql, zval *return_value)
// {

//     if (FireBird_Statement_prepare(return_value, sql)) {
//         return FAILURE;
//     }

//     return SUCCESS;
// }

PHP_METHOD(FireBird_Transaction, prepare)
{
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Statement_ce);
    FireBird_Statement___construct(return_value, ZEND_THIS);

    if (FireBird_Statement_prepare(return_value, sql)) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
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
    FireBird_Statement___construct(return_value, ZEND_THIS);

    if (FireBird_Statement_prepare(return_value, sql)) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }

    if (FireBird_Statement_execute(return_value, bind_args, num_bind_args)) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

int FireBird_Transaction_execute(zval *self, zend_string *sql)
{
    firebird_trans *tr = get_firebird_trans_from_zval(self);

    FBDEBUG("%s(tr=%p, tr->trptr=%p)", __func__, tr, tr->trptr);

    if (fbu_transaction_execute(tr, ZSTR_LEN(sql), ZSTR_VAL(sql))) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Transaction, execute)
{
    zval *bind_args;
    uint32_t num_bind_args;
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Transaction_execute(ZEND_THIS, sql)) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

#if 0
PHP_METHOD(FireBird_Transaction, execute_immediate)
{
    TODO("PHP_METHOD(FireBird_Transaction, execute_immediate)");
    zval *bind_args;
    uint32_t num_bind_args;
    char *sql;
    size_t sql_len;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STRING(sql, sql_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);

    firebird_stmt stmtb = {0};
    firebird_stmt *stmt = &stmtb;
    fbp_statement_ctor(stmt, tr);
    stmt->query = sql;

    ISC_STATUS isc_result;
    if (isc_dsql_allocate_statement(FBG(status), stmt->db_handle, &stmt->stmt_handle)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    if (num_bind_args > 0) {
        stmt->in_sqlda = emalloc(XSQLDA_LENGTH(num_bind_args));
        stmt->in_sqlda->sqln = num_bind_args;
        stmt->in_sqlda->sqld = num_bind_args;
        stmt->in_sqlda->version = SQLDA_CURRENT_VERSION;
        stmt->bind_buf = safe_emalloc(sizeof(firebird_bind_buf), num_bind_args, 0);

        // Here we need some kind of fbp_statement_bind() but in reverse -
        // populate in_sqlda types accordingly to bind_args types. Unfortunately
        // there is not much information to be extracted about table fields
        // involved in a query without isc_dsql_describe_bind() which requires
        // isc_dsql_prepare() to be called, but that beats the purpose of
        // immediate execution
        //
        // For now I will just parse NULLs and BLOBs. Other types will just go
        // as strings
        int is_blob;
        XSQLDA *sqlda = stmt->in_sqlda;
        ISC_QUAD bl_id;
        for (size_t i = 0; i < num_bind_args; ++i) {
            zval *b_var = &bind_args[i];
            XSQLVAR *var = &sqlda->sqlvar[i];

            FBDEBUG_NOFL("Arg: %d, type: %d", i, Z_TYPE_P(b_var));

            if (Z_TYPE_P(b_var) == IS_NULL) {
                FBDEBUG_NOFL("  is null");
                stmt->bind_buf[i].sqlind = -1;
                var->sqltype = SQL_TEXT | 1; // set bit 1 indicates NULL-able type
                var->sqldata = NULL;
                continue;
            }

            stmt->bind_buf[i].sqlind = 0;

            var->sqldata = (void*)&stmt->bind_buf[i].val;

            if (Z_TYPE_P(b_var) == IS_TRUE || Z_TYPE_P(b_var) == IS_FALSE) {
                FBDEBUG_NOFL("  is bool: %i", zend_is_true(b_var));
                var->sqltype = SQL_BOOLEAN;
                var->sqllen = 1;
                *(FB_BOOLEAN *)var->sqldata = zend_is_true(b_var) ? FB_TRUE : FB_FALSE;
                continue;
            }

            is_blob = 0;
            if (Z_TYPE_P(b_var) == IS_OBJECT) {
                FBDEBUG_NOFL("  is object");
                if (Z_OBJCE_P(b_var) == FireBird_Blob_Id_ce) {
                    bl_id = get_firebird_blob_id_from_zval(b_var)->bl_id;
                    is_blob = 1;
                } else if (Z_OBJCE_P(b_var) == FireBird_Blob_ce) {
                    bl_id = get_firebird_blob_from_zval(b_var)->bl_id;
                    is_blob = 1;
                }
            }

            if (is_blob) {
                FBDEBUG_NOFL("     is blob");
                var->sqltype = SQL_BLOB;
                stmt->bind_buf[i].val.qval = bl_id;
                continue;
            }

            convert_to_string(b_var);
            var->sqldata = Z_STRVAL_P(b_var);
            var->sqllen	 = Z_STRLEN_P(b_var);
            var->sqltype = SQL_TEXT;
            var->sqlsubtype = 0;
        }
    }

    if (fbp_statement_execute(stmt, bind_args, num_bind_args, FBP_STMT_EXECUTE_IMMEDIATE)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    fbp_statement_free(stmt);

    if (isc_dsql_free_statement(FBG(status), &stmt->stmt_handle, DSQL_drop)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
#endif

PHP_METHOD(FireBird_Transaction, open_blob)
{
    zval *Blob_Id;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Blob_Id, FireBird_Blob_Id_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Blob_ce);
    FireBird_Blob___construct(return_value, ZEND_THIS);

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);
    firebird_blob *blob = get_firebird_blob_from_zval(return_value);
    firebird_blob_id *blob_id = get_firebird_blob_id_from_zval(Blob_Id);

    if (fbu_blob_open(FBG(status), tr, blob_id->bl_id, blob)) {
        update_err_props(FBG(status), FireBird_Blob_ce, Blob);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

PHP_METHOD(FireBird_Transaction, create_blob)
{
    TODO("PHP_METHOD(FireBird_Transaction, create_blob)");
#if 0
    ZEND_PARSE_PARAMETERS_NONE();

    object_init_ex(return_value, FireBird_Blob_ce);
    FireBird_Blob___construct(return_value, ZEND_THIS);

    if (FireBird_Blob_create(return_value)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
#endif
}

PHP_METHOD(FireBird_Transaction, prepare_2pc)
{
    TODO("PHP_METHOD(FireBird_Transaction, prepare_2pc)");
#if 0
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);

    if (isc_prepare_transaction(FBG(status), tr->tr_handle)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    tr->is_prepared_2pc = 1;

    RETURN_TRUE;
#endif
}

static zend_object *FireBird_Transaction_create_object(zend_class_entry *ce)
{
    firebird_trans *tr = zend_object_alloc(sizeof(firebird_trans), ce);

    FBDEBUG("+%s(tr=%p)", __func__, tr);

    zend_object_std_init(&tr->std, ce);
    object_properties_init(&tr->std, ce);

    return &tr->std;
}

static void FireBird_Transaction_free_obj(zend_object *obj)
{
    firebird_trans *tr = get_firebird_trans_from_obj(obj);

    FBDEBUG("~%s(tr=%p, tr->trptr=%p)", __func__, tr, tr->trptr);

    if (tr->trptr) fbu_transaction_free(tr);

    zend_object_std_dtor(&tr->std);
}

static zval* FireBird_Transaction_read_property(zend_object *obj, zend_string *name, int type,
    void **cache_slot, zval *rv)
{
    firebird_trans *tr = get_firebird_trans_from_obj(obj);

    if (zend_string_equals_literal(name, "id")) {
        ZVAL_LONG(rv, tr->id);
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
