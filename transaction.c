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
#include "fbp_transaction.h"
#include "fbp_blob.h"

static zend_object_handlers object_handlers_FireBird_Transaction;

void FireBird_Transaction___construct(zval *Tr, zval *Database, zval *Builder)
{
    OBJ_SET(FireBird_Transaction_ce, Tr, "database", Database);
    if (Builder && !ZVAL_IS_NULL(Builder)) {
        OBJ_SET(FireBird_Transaction_ce, Tr, "builder", Builder);
    }

    fbp_transaction_ctor(
        get_firebird_trans_from_zval(Tr),
        get_firebird_db_from_zval(Database),
        Builder ? get_firebird_tbuilder_from_zval(Builder) : NULL
    );
}

PHP_METHOD(FireBird_Transaction, __construct)
{
    zval *Database, *Builder = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_OBJECT_OF_CLASS(Database, FireBird_Database_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_OBJECT_OF_CLASS(Builder, FireBird_TBuilder_ce)
    ZEND_PARSE_PARAMETERS_END();

    FireBird_Transaction___construct(ZEND_THIS, Database, Builder);
}

// void FireBird_Transaction_start(zval *Tr, zval *return_value)
// {
//     firebird_trans *tr = get_firebird_trans_from_zval(Tr);

//     if (fbu_start_transaction(FBG(status), tr)) {
//         update_err_props(FBG(status), FireBird_Transaction_ce, Tr);
//         RETURN_FALSE;
//     }

//     zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(Tr), "id", 2, (zend_long)tr->tr_id);

//     RETURN_TRUE;
// }

PHP_METHOD(FireBird_Transaction, start) {
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);

    if (fbu_start_transaction(FBG(status), tr)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, Tr);
        RETURN_FALSE;
    }

    zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(ZEND_THIS), "id", 2, (zend_long)tr->tr_id);

    RETURN_TRUE;
    // FireBird_Transaction_start(ZEND_THIS, return_value);
}

static void FireBird_Transaction_finalize(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);

    php_printf("FireBird_Transaction_finalize: %p\n", tr);

    if (fbu_finalize_transaction(FBG(status), tr, mode)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
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

int FireBird_Transaction_prepare(zval *Tr, zval *return_value, const ISC_SCHAR* sql)
{
    object_init_ex(return_value, FireBird_Statement_ce);
    FireBird_Statement___construct(return_value, Tr);

    if (FireBird_Statement_prepare(return_value, sql)) {
        zval_ptr_dtor(return_value);
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Transaction, prepare)
{
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Transaction_prepare(ZEND_THIS, return_value, ZSTR_VAL(sql))) {
        RETURN_FALSE;
    }
}

PHP_METHOD(FireBird_Transaction, query)
{
    zval *bind_args;
    uint32_t num_bind_args;
    zend_string *sql;
    zval Stmt;
    // firebird_trans *tr = get_firebird_trans_from_zval(ZEND_THIS);
    firebird_stmt *s;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(sql)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Transaction_prepare(ZEND_THIS, &Stmt, ZSTR_VAL(sql))) {
        RETURN_FALSE;
    }

    s = get_firebird_stmt_from_zval(&Stmt);

    switch(s->statement_type) {
        case isc_info_sql_stmt_select:
        case isc_info_sql_stmt_select_for_upd: {
            if (fbu_statement_bind(FBG(status), s, bind_args, num_bind_args)) {
                update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
                RETURN_FALSE;
            }

            if (fbu_open_cursor(FBG(status), s)) {
                update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
                RETURN_FALSE;
            }
        } break;

        case isc_info_sql_stmt_exec_procedure:
        case isc_info_sql_stmt_insert:
        case isc_info_sql_stmt_update:
        case isc_info_sql_stmt_delete: {
            if (fbu_statement_bind(FBG(status), s, bind_args, num_bind_args)) {
                update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
                RETURN_FALSE;
            }

            if (fbu_execute_statement(FBG(status), s)) {
                update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
                RETURN_FALSE;
            }

        } break;

        default:
            fbp_fatal("TODO: s->statement_type: %d", s->statement_type);
    }

    // if (FireBird_Statement_execute(&Stmt, bind_args, num_bind_args)) {
    //     update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
    //     zval_ptr_dtor(&Stmt);
    //     RETURN_FALSE;
    // }

    RETVAL_COPY(&Stmt);

    zval_ptr_dtor(&Stmt);
}

PHP_METHOD(FireBird_Transaction, execute_immediate)
{
    TODO("PHP_METHOD(FireBird_Transaction, execute_immediate)");
#if 0
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
#endif
}

PHP_METHOD(FireBird_Transaction, open_blob)
{
    zval *Blob_Id;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Blob_Id, FireBird_Blob_Id_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Blob_ce);
    FireBird_Blob___construct(return_value, ZEND_THIS);

    if (FireBird_Blob_open(return_value, Blob_Id)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

PHP_METHOD(FireBird_Transaction, create_blob)
{
    ZEND_PARSE_PARAMETERS_NONE();

    object_init_ex(return_value, FireBird_Blob_ce);
    FireBird_Blob___construct(return_value, ZEND_THIS);

    if (FireBird_Blob_create(return_value)) {
        update_err_props(FBG(status), FireBird_Transaction_ce, ZEND_THIS);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
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

static zend_object *new_FireBird_Transaction(zend_class_entry *ce)
{
    firebird_trans *tr = zend_object_alloc(sizeof(firebird_trans), ce);

    zend_object_std_init(&tr->std, ce);
    object_properties_init(&tr->std, ce);

    return &tr->std;
}

static void free_FireBird_Transaction(zend_object *obj)
{
    firebird_trans *tr = get_firebird_trans_from_obj(obj);

    php_printf("free_FireBird_Transaction: %p\n", tr->tra);

    // MAYBE: not to close automatically in some strict mode or smth
    // if(tr->tr_handle && *tr->tr_handle && !tr->is_prepared_2pc) {
    //     if(isc_rollback_transaction(FBG(status), tr->tr_handle)) {
    //         fbp_status_error(FBG(status));
    //     }
    // }
    if (tr->tra) {
        if (fbu_finalize_transaction(FBG(status), tr, FBP_TR_ROLLBACK)){
            fbp_status_error(FBG(status));
        }
    }

    zend_object_std_dtor(&tr->std);
}

void register_FireBird_Transaction_object_handlers() {
    FireBird_Transaction_ce->create_object = new_FireBird_Transaction;
    FireBird_Transaction_ce->default_object_handlers = &object_handlers_FireBird_Transaction;

    memcpy(&object_handlers_FireBird_Transaction, &std_object_handlers, sizeof(zend_object_handlers));

    object_handlers_FireBird_Transaction.offset = XtOffsetOf(firebird_trans, std);
    object_handlers_FireBird_Transaction.free_obj = free_FireBird_Transaction;
}

// void register_FireBird_Transaction_ce()
// {
//     zend_class_entry tmp_ce;
//     INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Transaction", FireBird_Transaction_methods);
//     FireBird_Transaction_ce = zend_register_internal_class(&tmp_ce);

//     DECLARE_PROP_OBJ(FireBird_Transaction_ce, database, FireBird\\Database, ZEND_ACC_PROTECTED_SET);
//     DECLARE_PROP_OBJ(FireBird_Transaction_ce, builder, FireBird\\TBuilder, ZEND_ACC_PROTECTED_SET);
//     DECLARE_PROP_LONG(FireBird_Transaction_ce, id, ZEND_ACC_PROTECTED_SET);
//     DECLARE_ERR_PROPS(FireBird_Transaction_ce);

//     zend_class_implements(FireBird_Transaction_ce, 1, FireBird_IError_ce);

//     FireBird_Transaction_ce->create_object = new_FireBird_Transaction;
//     FireBird_Transaction_ce->default_object_handlers = &object_handlers_FireBird_Transaction;

//     memcpy(&object_handlers_FireBird_Transaction, &std_object_handlers, sizeof(zend_object_handlers));

//     object_handlers_FireBird_Transaction.offset = XtOffsetOf(firebird_trans, std);
//     object_handlers_FireBird_Transaction.free_obj = free_FireBird_Transaction;
// }
