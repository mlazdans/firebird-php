#include "firebird_php.hpp"
#include "fbp/statement.hpp"
#include "firebird_utils.h"

extern "C" {

#include <ibase.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "ext/spl/spl_exceptions.h"

fbp_object_accessor(firebird_stmt);
static zend_object_handlers FireBird_Statement_object_handlers;

int FireBird_Statement___construct(zval *self, zval *transaction)
{
    firebird_stmt *stmt = get_firebird_stmt_from_zval(self);
    firebird_trans *tr = get_firebird_trans_from_zval(transaction);

    size_t sth;

    if (fbu_statement_init(tr->dbh, tr->trh, &sth)) {
        return FAILURE;
    }

    stmt->sth = sth;
    stmt->dbh = tr->dbh;
    stmt->trh = tr->trh;

    PROP_SET(FireBird_Statement_ce, self, "transaction", transaction);

    return SUCCESS;
}

PHP_METHOD(FireBird_Statement, __construct)
{
}

static void _FireBird_Statement_fetch(INTERNAL_FUNCTION_PARAMETERS, int return_type)
{
    zend_long flags = 0;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();

    // The row shape comes from the method, never from caller-supplied bits
    flags &= ~(zend_long)(FBP_FETCH_INDEXED | FBP_FETCH_HASHED);

    FireBird_Statement_fetch(ZEND_THIS, (int)flags | return_type, return_value);
}

PHP_METHOD(FireBird_Statement, fetch_row)
{
    _FireBird_Statement_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_FETCH_INDEXED);
}

PHP_METHOD(FireBird_Statement, fetch_array)
{
    _FireBird_Statement_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_FETCH_HASHED);
}

PHP_METHOD(FireBird_Statement, fetch_object)
{
    _FireBird_Statement_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBP_FETCH_HASHED);

    if (Z_TYPE_P(return_value) == IS_ARRAY) {
        convert_to_object(return_value);
    }
}

PHP_METHOD(FireBird_Statement, close_cursor)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);

    if (fbu_statement_close_cursor(stmt->dbh, stmt->sth)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Statement, free)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);

    if (fbu_statement_free(stmt->dbh, stmt->sth)) {
        RETURN_THROWS();
    }

    stmt->sth = 0;
}

int FireBird_Statement_execute(zval *self, zval *bind_args, uint32_t num_bind_args)
{
    firebird_stmt *stmt = get_firebird_stmt_from_zval(self);

    if (fbu_statement_bind(stmt->dbh, stmt->sth, bind_args, num_bind_args)) {
        return FAILURE;
    }

    if (fbu_statement_execute(stmt->dbh, stmt->sth)) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Statement, execute)
{
    zval *bind_args;
    uint32_t num_bind_args;

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    if (FireBird_Statement_execute(ZEND_THIS, bind_args, num_bind_args)) {
        RETURN_THROWS();
    }
}

int FireBird_Statement_prepare(zval *self, zend_string *sql)
{
    firebird_stmt *stmt = get_firebird_stmt_from_zval(self);

    if (fbu_statement_prepare(stmt->dbh, stmt->sth, ZSTR_LEN(sql), ZSTR_VAL(sql))) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(FireBird_Statement, get_var_info_in)
{
    zend_long index;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);

    object_init_ex(return_value, FireBird_Var_Info_ce);
    if (fbu_statement_get_var_info(stmt->dbh, stmt->sth, 1, index, return_value)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Statement, get_var_info_out)
{
    zend_long index;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);

    object_init_ex(return_value, FireBird_Var_Info_ce);
    if (fbu_statement_get_var_info(stmt->dbh, stmt->sth, 0, index, return_value)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Statement, set_name)
{
    TODO("PHP_METHOD(FireBird_Statement, set_name)");
#if 0
    firebird_stmt *stmt = get_firebird_stmt_from_zval(ZEND_THIS);
    char *name;
    size_t name_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(name, name_len)
    ZEND_PARSE_PARAMETERS_END();

    if (isc_dsql_set_cursor_name(FBG(status), &stmt->stmt_handle, name, 0)) {
        update_err_props(FBG(status), FireBird_Statement_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    stmt->name = name;

    zend_update_property_string(FireBird_Statement_ce, Z_OBJ_P(ZEND_THIS), "name", sizeof("name") - 1, name);

    RETURN_TRUE;
#endif
}

static zend_object *FireBird_Statement_create_object(zend_class_entry *ce)
{
    firebird_stmt *s = (firebird_stmt *)zend_object_alloc(sizeof(firebird_stmt), ce);

    FBDEBUG("+%s(stmt=%p)", __func__, s);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Statement_free_obj(zend_object *obj)
{
    firebird_stmt *stmt = get_firebird_stmt_from_obj(obj);

    FBDEBUG("~%s(sth=%zu)", __func__, stmt->sth);

    if (fbu_is_valid_sth(stmt->dbh, stmt->sth)) {
        fbu_statement_free(stmt->dbh, stmt->sth);
    }

    zend_object_std_dtor(&stmt->std);
}

static zval* FireBird_Statement_read_property(zend_object *obj, zend_string *name, int type,
    void **cache_slot, zval *rv)
{
    bool is_virtual =
        zend_string_equals_literal(name, "name") ||
        zend_string_equals_literal(name, "in_vars_count") ||
        zend_string_equals_literal(name, "out_vars_count") ||
        zend_string_equals_literal(name, "insert_count") ||
        zend_string_equals_literal(name, "update_count") ||
        zend_string_equals_literal(name, "delete_count") ||
        zend_string_equals_literal(name, "affected_count");

    if (!is_virtual) {
        return zend_std_read_property(obj, name, type, cache_slot, rv);
    }

    // Looked up on every read: the C++ object is gone once the statement is freed or
    // its database disconnected, and get_info() then throws instead of reading freed memory
    firebird_stmt *stmt = get_firebird_stmt_from_obj(obj);
    const firebird_stmt_info *info;

    if (fbu_statement_get_info(stmt->dbh, stmt->sth, &info)) {
        return &EG(uninitialized_zval);
    }

    if (zend_string_equals_literal(name, "name")) {
        ZVAL_STRING(rv, info->name ? info->name : "");
    } else if (zend_string_equals_literal(name, "in_vars_count")) {
        ZVAL_LONG(rv, info->in_vars_count);
    } else if (zend_string_equals_literal(name, "out_vars_count")) {
        ZVAL_LONG(rv, info->out_vars_count);
    } else if (zend_string_equals_literal(name, "insert_count")) {
        ZVAL_LONG(rv, info->insert_count);
    } else if (zend_string_equals_literal(name, "update_count")) {
        ZVAL_LONG(rv, info->update_count);
    } else if (zend_string_equals_literal(name, "delete_count")) {
        ZVAL_LONG(rv, info->delete_count);
    } else {
        ZVAL_LONG(rv, info->affected_count);
    }

    return rv;
}

void register_FireBird_Statement_object_handlers()
{
    FireBird_Statement_ce->default_object_handlers = &FireBird_Statement_object_handlers;
    FireBird_Statement_ce->create_object = FireBird_Statement_create_object;

    memcpy(&FireBird_Statement_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Statement_object_handlers.offset = XtOffsetOf(firebird_stmt, std);
    FireBird_Statement_object_handlers.free_obj = FireBird_Statement_free_obj;
    FireBird_Statement_object_handlers.read_property = FireBird_Statement_read_property;
}

void FireBird_Statement_fetch(zval *self, int flags, zval *return_value)
{
    HashTable *ht = NULL;
    firebird_stmt *stmt = get_firebird_stmt_from_zval(self);
    int istatus;

    FBDEBUG("%s(sth=%zu)", __func__,  stmt->sth);

    if (fbu_statement_fetch_next(stmt->dbh, stmt->sth, &istatus)) {
        RETURN_THROWS();
    }

    if (istatus != IStatus::RESULT_OK) {
        RETURN_FALSE;
    }

    if (fbu_statement_output_buffer_to_array(stmt->dbh, stmt->sth, flags, &ht)) {
        RETURN_THROWS();
    }

    RETURN_ARR(ht);
}

}
