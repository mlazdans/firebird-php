#include "firebird_php.hpp"
#include "firebird_utils.h"

extern "C" {
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "ext/spl/spl_exceptions.h"

fbp_object_accessor(firebird_db);
static zend_object_handlers FireBird_Database_object_handlers;

// Flags used by createDatabase() jrd/jrd.cpp
// ✅ dpb_sweep_interval
// dpb_length
// dpb_auth_block
// ✅ dpb_sql_dialect
// dpb_org_filename
// dpb_utf8_filename
// dpb_owner
// dpb_remote_address
// dpb_working_directory
// dpb_sec_attach
// dpb_map_attach
// dpb_gbak_attach
// dpb_no_db_triggers
// dpb_interp
// ✅ dpb_page_size
// ✅ dpb_overwrite
// ✅ dpb_set_page_buffers
// dpb_set_no_reserve
// ✅ dpb_set_db_charset
// dpb_online
// dpb_shutdown
// dpb_activate_shadow
// dpb_parallel_workers
// dpb_set_db_readonly
// dpb_set_db_replica
// dpb_replica_mode
// dpb_session_tz
// dpb_set_force_write
// ✅ dpb_force_write

firebird_xpb_zmap fbp_database_create_zmap = XPB_ZMAP_INIT(
    ((const uint8_t[]){
        isc_dpb_user_name, isc_dpb_password, isc_dpb_set_db_charset, isc_dpb_sweep_interval,
        isc_dpb_set_page_buffers, isc_dpb_page_size, isc_dpb_force_write, isc_dpb_overwrite,
        isc_dpb_connect_timeout
    }),
    ((const char *[]){
        "user_name", "password", "set_db_charset", "sweep_interval",
        "set_page_buffers", "page_size", "force_write", "overwrite",
        "timeout"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_BOOL, MAY_BE_BOOL,
        MAY_BE_LONG
    })
);

firebird_xpb_zmap fbp_database_connect_zmap = XPB_ZMAP_INIT(
    ((const uint8_t[]){
        isc_dpb_user_name, isc_dpb_password, isc_dpb_lc_ctype, isc_dpb_sql_role_name, isc_dpb_num_buffers, isc_dpb_connect_timeout
    }),
    ((const char *[]){
        "user_name", "password", "charset", "role_name", "num_buffers", "timeout"
    }),
    ((uint32_t []) {
        MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_LONG, MAY_BE_LONG
    })
);

firebird_xpb_zmap fbp_database_info_zmap = XPB_ZMAP_INIT(
    ((const uint8_t[]){
        isc_info_reads, isc_info_writes, isc_info_fetches, isc_info_marks,
        isc_info_page_size, isc_info_num_buffers, isc_info_current_memory, isc_info_max_memory,

        isc_info_allocation, isc_info_attachment_id, isc_info_read_seq_count, isc_info_read_idx_count, isc_info_insert_count,
        isc_info_update_count, isc_info_delete_count, isc_info_backout_count, isc_info_purge_count, isc_info_expunge_count,

        isc_info_isc_version, isc_info_firebird_version, isc_info_limbo,
        isc_info_sweep_interval, isc_info_ods_version, isc_info_ods_minor_version, isc_info_no_reserve,

        isc_info_forced_writes, isc_info_set_page_buffers, isc_info_db_sql_dialect, isc_info_db_read_only, isc_info_db_size_in_pages,
        isc_info_oldest_transaction, isc_info_oldest_active, isc_info_oldest_snapshot, isc_info_next_transaction,

        isc_info_creation_date, isc_info_db_file_size, fb_info_pages_used, fb_info_pages_free,

        fb_info_ses_idle_timeout_db, fb_info_ses_idle_timeout_att, fb_info_ses_idle_timeout_run, fb_info_conn_flags,
        fb_info_crypt_key, fb_info_crypt_state, fb_info_statement_timeout_db, fb_info_statement_timeout_att,
        fb_info_protocol_version, fb_info_crypt_plugin, fb_info_wire_crypt,

        fb_info_next_attachment, fb_info_next_statement, fb_info_db_guid, fb_info_db_file_id,
        fb_info_replica_mode, fb_info_username, fb_info_sqlrole, fb_info_parallel_workers
    }),
    ((const char *[]){
        "reads", "writes", "fetches", "marks",
        "page_size", "num_buffers", "current_memory", "max_memory",

        "allocation", "attachment_id", "read_seq_count", "read_idx_count", "insert_count",
        "update_count", "delete_count", "backout_count", "purge_count", "expunge_count",

        "isc_version", "firebird_version", "limbo",
        "sweep_interval", "ods_version", "ods_minor_version", "no_reserve",

        "forced_writes", "set_page_buffers", "db_sql_dialect", "db_read_only", "db_size_in_pages",
        "oldest_transaction", "oldest_active", "oldest_snapshot", "next_transaction",

        "creation_date", "db_file_size", "pages_used", "pages_free",

        "ses_idle_timeout_db", "ses_idle_timeout_att", "ses_idle_timeout_run", "conn_flags",
        "crypt_key", "crypt_state", "statement_timeout_db", "statement_timeout_att", "protocol_version", "crypt_plugin", "wire_crypt",

        "next_attachment", "next_statement", "db_guid", "db_file_id",
        "replica_mode", "username", "sqlrole", "parallel_workers"

    }),
    ((uint32_t []) {
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,

        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,

        MAY_BE_ARRAY, MAY_BE_ARRAY, MAY_BE_ARRAY,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,

        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,

        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,

        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_STRING, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_STRING, MAY_BE_STRING,

        MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG, MAY_BE_LONG,
        MAY_BE_LONG, MAY_BE_STRING, MAY_BE_STRING, MAY_BE_LONG,
    })
);

/**
 * zval* Args intanceof Create_Args|Connect_Args
 */
// int fbp_database_build_dpb(zend_class_entry *ce, zval *Args, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written)
// {
//     struct IMaster* master = fb_get_master_interface();
//     struct IStatus* st = IMaster_getStatus(master);
//     struct IUtil* utl = IMaster_getUtilInterface(master);
//     struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_DPB, NULL, 0);

//     zend_property_info *prop_info = NULL;
//     zend_string *prop_name = NULL;
//     zval rv, *val, *checkval;
//     int i;

//     IXpbBuilder_insertTag(xpb, st, isc_dpb_version2);
//     IXpbBuilder_insertInt(xpb, st, isc_dpb_sql_dialect, SQL_DIALECT_CURRENT);

//     fbp_insert_xpb_from_zmap(ce, Args, xpb_zmap, xpb, st);

// #if FB_API_VER >= 40
//     // Do not handle directly INT128 or DECFLOAT, convert to VARCHAR at server instead
//     IXpbBuilder_insertString(xpb, st, isc_dpb_set_bind, "int128 to varchar;decfloat to varchar");
// #endif

//     *num_dpb_written = IXpbBuilder_getBufferLength(xpb, st);
//     *dpb_buf = IXpbBuilder_getBuffer(xpb, st);

//     // Needed? Not needed?
//     // IXpbBuilder_dispose(xpb);
//     // IStatus_dispose(st);

//     return SUCCESS;
// }

PHP_METHOD(FireBird_Database, connect)
{
    zval *Connect_Args;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Connect_Args, FireBird_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Database_ce);
    firebird_db *db = get_firebird_db_from_zval(return_value);

    if (fbu_database_init(&db->dbh)) {
        RETURN_THROWS();
    }

    if (fbu_database_connect(db->dbh, Connect_Args)) {
        RETURN_THROWS();
    }

    PROP_SET(FireBird_Database_ce, return_value, "args", Connect_Args);
}

PHP_METHOD(FireBird_Database, create)
{
    zval *Create_Args = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(Create_Args, FireBird_Create_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Database_ce);
    firebird_db *db = get_firebird_db_from_zval(return_value);

    if (fbu_database_init(&db->dbh)) {
        RETURN_THROWS();
    }

    if (fbu_database_create(db->dbh, Create_Args)) {
        RETURN_THROWS();
    }

    PROP_SET(FireBird_Database_ce, return_value, "args", Create_Args);
}

PHP_METHOD(FireBird_Database, execute_create)
{
     zend_string *sql;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(sql)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, FireBird_Database_ce);
    firebird_db *db = get_firebird_db_from_zval(return_value);

    if (fbu_database_init(&db->dbh)) {
        RETURN_THROWS();
    }

    if (fbu_database_execute_create(db->dbh, ZSTR_LEN(sql), ZSTR_VAL(sql))) {
        RETURN_THROWS();
    }
}

static firebird_db *FireBird_Database_get_db_or_throw(zval *dbobj)
{
    firebird_db *db = get_firebird_db_from_zval(dbobj);

    return db;
}

PHP_METHOD(FireBird_Database, drop)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_db *db = get_firebird_db_from_zval(ZEND_THIS);

    if (fbu_database_drop(db->dbh)) {
        RETURN_THROWS();
    }

    db->dbh = 0;
}

PHP_METHOD(FireBird_Database, get_info)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_db *db = get_firebird_db_from_zval(ZEND_THIS);

    object_init_ex(return_value, FireBird_Db_Info_ce);
    if (fbu_database_get_info(db->dbh, return_value)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Database, on_event)
{
    TODO("Database->on_event");
#if 0
    char *name;
    size_t name_len;

    firebird_db *db = get_firebird_db_from_zval(ZEND_THIS);

    // zval ievent;
    // object_init_ex(&ievent, FireBird_Event_ce);
    // firebird_event *event = get_firebird_event_from_zval(&ievent);
    firebird_event *event = emalloc(sizeof(firebird_event));

    // zval *f;
    // zend_fcall_info fci;
    // zend_fcall_info_cache fcc;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(name, name_len)
        // Z_PARAM_OBJECT_OF_CLASS(f, zend_ce_fiber)
        // Z_PARAM_FUNC_NO_TRAMPOLINE_FREE(event->fci, event->fcc)
        Z_PARAM_FUNC(event->fci, event->fcc) // TODO: check closure's signature (fcc.function_handler->common.num_args)
    ZEND_PARSE_PARAMETERS_END();

    ZVAL_NULL(&event->retval);
    event->fci.retval = &event->retval;

    event->state = NEW;
    event->db_handle = &db->db_handle;
    event->name = name; // TODO: should increase ref count or not?
    event->posted_count = 0;
    // event->fiber = Z_FIBER_P(f);

    event->buff_len = isc_event_block(&event->event_buffer, &event->result_buffer, 1, name);
    event->next = fb_events.events;

    FBDEBUG("Events: alloceted event: '%s', event_ptr: %p", name, event);

    if (isc_que_events(FBG(status), event->db_handle, &event->event_id, event->buff_len, event->event_buffer, event_ast_routine, NULL)) {
        update_err_props(FBG(status), FireBird_Database_ce, ZEND_THIS);
        // zval_ptr_dtor(&ievent);
        RETURN_FALSE;
    }

    fb_events.events = event;
    fb_events.count++;

    // ZVAL_COPY(&event->instance, &ievent);
    // zval_ptr_dtor(&ievent);

    RETURN_TRUE;
#endif
}

PHP_METHOD(FireBird_Database, new_transaction)
{
    ZEND_PARSE_PARAMETERS_NONE();

    object_init_ex(return_value, FireBird_Transaction_ce);

    if (FireBird_Transaction___construct(return_value, ZEND_THIS)) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Database, start_transaction)
{
    zval *builder = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_OBJECT_OF_CLASS(builder, FireBird_TBuilder_ce)
    ZEND_PARSE_PARAMETERS_END();

    FBDEBUG("%s(builder=%p)", __func__, builder);

    object_init_ex(return_value, FireBird_Transaction_ce);

    if (FireBird_Transaction___construct(return_value, ZEND_THIS) ||
        FireBird_Transaction_start(return_value, builder)
    ) {
        RETURN_THROWS();
    }
}

PHP_METHOD(FireBird_Database, disconnect)
{
    ZEND_PARSE_PARAMETERS_NONE();

    firebird_db *db = get_firebird_db_from_zval(ZEND_THIS);

    if (fbu_database_disconnect(db->dbh)) {
        RETURN_THROWS();
    }

    db->dbh = 0;
}

void FireBird_Database_reconnect_transaction(zval *Db, zval *return_value, zend_long id)
{
    TODO("FireBird_Database_reconnect_transaction");
#if 0
    object_init_ex(return_value, FireBird_Transaction_ce);
    FireBird_Transaction___construct(return_value, Db, NULL);
    firebird_trans *tr = get_firebird_trans_from_zval(return_value);

    FBDEBUG("Connection, reconnect_transaction: %d", id);
    if (fbp_transaction_reconnect(tr, id)) {
        update_err_props(FBG(status), FireBird_Database_ce, Db);
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }

    zend_update_property_long(FireBird_Transaction_ce, Z_OBJ_P(return_value), "id", 2, (zend_long)tr->tr_id);
#endif
}

PHP_METHOD(FireBird_Database, reconnect_transaction)
{
    TODO("PHP_METHOD(FireBird_Database, reconnect_transaction)");
#if 0
    zend_long id;
    zval rv, *database;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(id)
    ZEND_PARSE_PARAMETERS_END();

    FireBird_Database_reconnect_transaction(ZEND_THIS, return_value, id);
#endif
}

PHP_METHOD(FireBird_Database, get_limbo_transactions)
{
    TODO("PHP_METHOD(FireBird_Database, get_limbo_transactions)");
#if 0
    zend_long max_count;
    zval rv, tr_id;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(max_count)
    ZEND_PARSE_PARAMETERS_END();

    if (max_count < 1) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Invalid max_count argument. Expected max_count value > 0");
        RETURN_THROWS();
    }

    if (max_count > 0x7FFF) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Invalid max_count argument. max_count value capped at %d", 0x7FFF);
        RETURN_THROWS();
    }

    char *info_resp;
    size_t info_resp_capacity;
    char stack_buff[TRANS_ID_SIZE * TRANS_MAX_STACK_COUNT];

    if (max_count <= TRANS_MAX_STACK_COUNT) {
        info_resp_capacity = sizeof(stack_buff);
        info_resp = stack_buff;
    } else {
        info_resp_capacity = TRANS_ID_SIZE * max_count;
        info_resp = (char *)emalloc(info_resp_capacity);
    }

    static char info_req[] = { isc_info_limbo, isc_info_end };

    firebird_db *db = get_firebird_db_from_zval(ZEND_THIS);
    zval db_info = { 0 };

    object_init_ex(&db_info, FireBird_Db_Info_ce);

    if (fbp_database_get_info(db, &db_info, sizeof(info_req), info_req, info_resp_capacity, info_resp, max_count)) {
        // update_err_props(FBG(status), FireBird_Database_ce, ZEND_THIS);
        RETVAL_FALSE;
        goto free;
    }

    ZVAL_COPY_VALUE(return_value, PROP_GET(FireBird_Db_Info_ce, &db_info, "limbo"));

free:
    zval_ptr_dtor(&db_info);
    if (max_count > TRANS_MAX_STACK_COUNT) {
        efree(info_resp);
    }
#endif
}

PHP_METHOD(FireBird_Database, __construct)
{
}

static zend_object *FireBird_Database_create_object(zend_class_entry *ce)
{
    firebird_db *db = (firebird_db *)zend_object_alloc(sizeof(firebird_db), ce);

    FBDEBUG("+%s(db=%p)", __func__, db);

    zend_object_std_init(&db->std, ce);
    object_properties_init(&db->std, ce);

    return &db->std;
}

static void FireBird_Database_free_obj(zend_object *obj)
{
    firebird_db *db = get_firebird_db_from_obj(obj);

    if (fbu_is_valid_dbh(db->dbh)) {
        fbu_database_free(db->dbh);
        db->dbh = 0;
    }

    zend_object_std_dtor(&db->std);
}

void register_FireBird_Database_object_handlers() {
    FireBird_Database_ce->create_object = FireBird_Database_create_object;
    FireBird_Database_ce->default_object_handlers = &FireBird_Database_object_handlers;

    memcpy(&FireBird_Database_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Database_object_handlers.offset = XtOffsetOf(firebird_db, std);
    FireBird_Database_object_handlers.free_obj = FireBird_Database_free_obj;
}


// void register_FireBird_Database_ce()
// {
//     zend_class_entry tmp_ce;
//     INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Database", FireBird_Database_methods);
//     FireBird_Database_ce = zend_register_internal_class(&tmp_ce);

//     {
//         zend_type_list *list = malloc(ZEND_TYPE_LIST_SIZE(2)); // keep malloc here
//         list->num_types = 2;
//         list->types[0] = (zend_type) ZEND_TYPE_INIT_CLASS(zend_string_init("FireBird\\Create_Args", sizeof("FireBird\\Create_Args") - 1, 1), 0, 0);
//         list->types[1] = (zend_type) ZEND_TYPE_INIT_CLASS(zend_string_init("FireBird\\Connect_Args", sizeof("FireBird\\Connect_Args") - 1, 1), 0, 0);
//         zend_type prop_type = ZEND_TYPE_INIT_UNION(list, 0);
//         zend_string *prop_name = zend_string_init("args", sizeof("args") - 1, 1);
//         zval def;
//         ZVAL_UNDEF(&def);

//         zend_declare_typed_property(FireBird_Database_ce, prop_name, &def, ZEND_ACC_PUBLIC, NULL, prop_type);
//         zend_string_release(prop_name);
//     }

//     DECLARE_ERR_PROPS(FireBird_Database_ce);

//     zend_class_implements(FireBird_Database_ce, 1, FireBird_IError_ce);
// }

// void register_FireBird_Db_Info_ce()
// {
//     zend_class_entry tmp_ce;
//     INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Db_Info", NULL);
//     FireBird_Db_Info_ce = zend_register_internal_class(&tmp_ce);

//     fbp_declare_props_from_zmap(FireBird_Db_Info_ce, &fbp_database_info_zmap);
// }

}
