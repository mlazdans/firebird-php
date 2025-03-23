#include <ibase.h>
#include <firebird/fb_c_api.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Transaction_ce;
static zend_object_handlers FireBird_Transaction_object_handlers;

static void transaction_process(INTERNAL_FUNCTION_PARAMETERS, int mode);

#define ROLLBACK    0
#define COMMIT      1
#define RETAIN      2

void transaction_ctor(firebird_trans *tr, firebird_db *db)
{
    tr->tr_handle = 0;
    tr->tr_id = 0;
    tr->is_prepared_2pc = 0;
    tr->db_handle = &db->db_handle;
}

void transaction__construct(zval *tr, zval *database, zval *builder)
{
    zend_update_property(FireBird_Transaction_ce, O_SET(tr, database));
    if (builder) {
        zend_update_property(FireBird_Transaction_ce, O_SET(tr, builder));
    }

    transaction_ctor(Z_TRANSACTION_P(tr), Z_DB_P(database));
}

PHP_METHOD(Transaction, __construct) {
}

int transaction_start(ISC_STATUS_ARRAY status, zval *tr_o)
{
    zval rv, *builder_o = NULL;
    ISC_STATUS result;
    char tpb[TPB_MAX_SIZE];
    unsigned short tpb_len = 0;

    builder_o = zend_read_property(FireBird_TBuilder_ce, O_GET(tr_o, builder), 1, &rv);
    if (Z_TYPE_P(builder_o) != IS_NULL) {
        firebird_tbuilder *builder = Z_TBUILDER_P(builder_o);
        tbuilder_populate_tpb(builder, tpb, &tpb_len);
        ZEND_ASSERT(tpb_len <= sizeof(tpb));
    }

    firebird_trans *tr = Z_TRANSACTION_P(tr_o);

    // TODO: isc_start_multiple, isc_prepare_transaction, isc_prepare_transaction2
    // isc_start_multiple()       - Begins a new transaction against multiple databases.
    // isc_prepare_transaction()  - Executes the first phase of a two-phase commit against multiple databases.
    // isc_prepare_transaction2() - Performs the first phase of a two-phase commit for multi-database transactions.
    if(isc_start_transaction(status, &tr->tr_handle, 1, tr->db_handle, tpb_len, tpb)) {
        return FAILURE;
    }

    if(transaction_get_info(status, tr)) {
        return FAILURE;
    }

    zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(tr_o), "id", 2, (zend_long)tr->tr_id);

    return SUCCESS;
}

PHP_METHOD(Transaction, start) {
    ISC_STATUS_ARRAY status;

    ZEND_PARSE_PARAMETERS_NONE();

    if(FAILURE == transaction_start(status, ZEND_THIS)) {
        update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_METHOD(Transaction, commit) {
    transaction_process(INTERNAL_FUNCTION_PARAM_PASSTHRU, COMMIT);
}

PHP_METHOD(Transaction, commit_ret) {
    transaction_process(INTERNAL_FUNCTION_PARAM_PASSTHRU, COMMIT | RETAIN);
}

PHP_METHOD(Transaction, rollback) {
    transaction_process(INTERNAL_FUNCTION_PARAM_PASSTHRU, ROLLBACK);
}

PHP_METHOD(Transaction, rollback_ret) {
    transaction_process(INTERNAL_FUNCTION_PARAM_PASSTHRU, ROLLBACK | RETAIN);
}

int prepare_for_transaction(INTERNAL_FUNCTION_PARAMETERS, const ISC_SCHAR* sql)
{
    ISC_STATUS_ARRAY status;

    object_init_ex(return_value, FireBird_Statement_ce);
    statement_ctor(return_value, ZEND_THIS);
    if (FAILURE == statement_prepare(status, return_value, sql)) {
        ISC_INT64 error_code_long = update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);

        // Do we CREATE DATABASE?
        if (error_code_long == isc_dsql_crdb_prepare_err) {
            fbp_error("CREATE DATABASE detected on active connection. Use Database::create() instead.");
        }

        zval_ptr_dtor(return_value);
        return FAILURE;
    }

    return SUCCESS;
}

PHP_METHOD(Transaction, prepare)
{
    zval rv;
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == prepare_for_transaction(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZSTR_VAL(sql))) {
        RETURN_FALSE;
    }
}

PHP_METHOD(Transaction, query)
{
    zval rv, *bind_args;
    uint32_t num_bind_args;
    zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(sql)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', bind_args, num_bind_args)
    ZEND_PARSE_PARAMETERS_END();

    if(FAILURE == prepare_for_transaction(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZSTR_VAL(sql))) {
        RETURN_FALSE;
    }

    if (FAILURE == statement_execute(return_value, bind_args, num_bind_args, FireBird_Transaction_ce, ZEND_THIS)) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

PHP_METHOD(Transaction, open_blob)
{
    zval *id;
    ISC_STATUS_ARRAY status;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(id, FireBird_Blob_Id_ce)
    ZEND_PARSE_PARAMETERS_END();

    firebird_blob_id *blob_id = Z_BLOB_ID_P(id);

    blob___construct(return_value, ZEND_THIS);
    firebird_blob *blob = Z_BLOB_P(return_value);

    blob->bl_id = blob_id->bl_id;

    // TODO: pass ce and obj to these functions to reflect more precisely where the error was
    if (FAILURE == blob_open(status, blob)) {
        update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

PHP_METHOD(Transaction, create_blob)
{
    zend_string *id;
    ISC_STATUS_ARRAY status;

    ZEND_PARSE_PARAMETERS_NONE();

    blob___construct(return_value, ZEND_THIS);
    firebird_blob *blob = Z_BLOB_P(return_value);

    if (FAILURE == blob_create(status, blob)) {
        update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

PHP_METHOD(Transaction, prepare_2pc)
{
    ISC_STATUS_ARRAY status;
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);

    ZEND_PARSE_PARAMETERS_NONE();

    if (isc_prepare_transaction(status, &tr->tr_handle)) {
        update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    tr->is_prepared_2pc = 1;

    // if (fb_disconnect_transaction(status, &tr->tr_handle)) {
    //     update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
    //     RETURN_FALSE;
    // }

    RETURN_TRUE;
}

const zend_function_entry FireBird_Transaction_methods[] = {
    PHP_ME(Transaction, __construct, arginfo_none, ZEND_ACC_PRIVATE)
    PHP_ME(Transaction, start, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, commit, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, commit_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, rollback, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, rollback_ret, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, prepare, arginfo_FireBird_Transaction_prepare, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, query, arginfo_FireBird_Transaction_query, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, open_blob, arginfo_FireBird_Transaction_open_blob, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, create_blob, arginfo_FireBird_Transaction_create_blob, ZEND_ACC_PUBLIC)
    PHP_ME(Transaction, prepare_2pc, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
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
    FBDEBUG("FireBird_Transaction_free_obj");

    firebird_trans *tr = Z_TRANSACTION_O(obj);

    // Let Firebird auto handle this
    // if(tr->tr_handle && !tr->disconnected) {
    //     ISC_STATUS_ARRAY status;
    //     if(isc_rollback_transaction(status, &tr->tr_handle)) {
    //         status_fbp_error(status);
    //     } else {
    //         tr->tr_handle = 0;
    //     }
    // }

    zend_object_std_dtor(&tr->std);
}

void register_FireBird_Transaction_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Transaction", FireBird_Transaction_methods);
    FireBird_Transaction_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_PROP_OBJ(FireBird_Transaction_ce, database, FireBird\\Database, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_OBJ(FireBird_Transaction_ce, builder, FireBird\\TBuilder, ZEND_ACC_PROTECTED_SET);
    DECLARE_PROP_LONG(FireBird_Transaction_ce, id, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Transaction_ce);

    zend_class_implements(FireBird_Transaction_ce, 1, FireBird_IError_ce);

    FireBird_Transaction_ce->create_object = FireBird_Transaction_create;
    FireBird_Transaction_ce->default_object_handlers = &FireBird_Transaction_object_handlers;

    memcpy(&FireBird_Transaction_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Transaction_object_handlers.offset = XtOffsetOf(firebird_trans, std);
    FireBird_Transaction_object_handlers.free_obj = FireBird_Transaction_free_obj;
}

static void transaction_process(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
    ZEND_PARSE_PARAMETERS_NONE();

    zval rv, *val;
    ISC_STATUS result;
    ISC_STATUS_ARRAY status;
    firebird_trans *tr = Z_TRANSACTION_P(ZEND_THIS);

    if (mode == COMMIT) {
        result = isc_commit_transaction(status, &tr->tr_handle);
    } else if (mode == ROLLBACK | RETAIN) {
        result = isc_rollback_retaining(status, &tr->tr_handle);
    } else if (mode == COMMIT | RETAIN) {
        result = isc_commit_retaining(status, &tr->tr_handle);
    } else {
        result = isc_rollback_transaction(status, &tr->tr_handle);
    }

    if (result) {
        update_err_props(status, FireBird_Transaction_ce, ZEND_THIS);
        RETVAL_FALSE;
    } else {
        RETVAL_TRUE;
    }
}

int transaction_get_info(ISC_STATUS_ARRAY status, firebird_trans *tr)
{
    static char info_req[] = { isc_info_tra_id };
    char info_resp[(sizeof(ISC_INT64) * 2)] = { 0 };

    if (isc_transaction_info(status, &tr->tr_handle, sizeof(info_req), info_req, sizeof(info_resp), info_resp)) {
        return FAILURE;
    }

    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* dpb;

    dpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_INFO_RESPONSE, info_resp, sizeof(info_resp));

    ISC_INT64 val, len, total_len = 0;
    const char *str;
    unsigned char tag;

    FBDEBUG("Parsing Transaction info buffer");
    for (IXpbBuilder_rewind(dpb, st); !IXpbBuilder_isEof(dpb, st); IXpbBuilder_moveNext(dpb, st)) {
        tag = IXpbBuilder_getTag(dpb, st); total_len++;
        len = IXpbBuilder_getLength(dpb, st); total_len += 2;
        total_len += len;

        switch(tag) {
            case isc_info_end: break;

            case isc_info_tra_id: {
                val = IXpbBuilder_getBigInt(dpb, st);
                FBDEBUG_NOFL("  tag: %d len: %d val: %d", tag, len, val);
                tr->tr_id = (ISC_UINT64)val;
            } break;

            case fb_info_tra_dbpath: {
                str = IXpbBuilder_getString(dpb, st);
                FBDEBUG_NOFL("  tag: %d len: %d val: %s", tag, len, str);
            } break;

            case isc_info_truncated: {
                fbp_error("Transaction info buffer error: truncated");
            } return FAILURE;

            case isc_info_error: {
                fbp_error("Transaction info buffer error");
            } return FAILURE;

            default: {
                fbp_fatal("BUG! Unhandled Transaction info tag: %d", tag);
            } break;
        }
    }

    return SUCCESS;
}
