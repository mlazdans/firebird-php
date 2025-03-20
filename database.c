#include <firebird/fb_c_api.h>
#include "php.h"
#include "zend_exceptions.h"
#include "zend_attributes.h"
#include "php_firebird_includes.h"

zend_class_entry *FireBird_Database_ce, *FireBird_Db_Info_ce;
static zend_object_handlers FireBird_Database_object_handlers;

int database_connect(ISC_STATUS_ARRAY status, zval *db_o, zval *args_o)
{
    zval rv, *database;
    const char *dpb_buffer;
    short num_dpb_written;
    firebird_db *db = Z_DB_P(db_o);

    // TODO: check if instance of Create_Args
    database = zend_read_property(FireBird_Connect_Args_ce, O_GET(args_o, database), 0, &rv);

    if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
        return FAILURE;
    }

    if (FAILURE == database_build_dpb(FireBird_Connect_Args_ce, args_o, &database_connect_zmap, &dpb_buffer, &num_dpb_written)) {
        return FAILURE;
    }

    FBDEBUG("connection_connect: %s", Z_STRVAL_P(database));
    if (isc_attach_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &db->db_handle, num_dpb_written, dpb_buffer)) {
        return FAILURE;
    }
    FBDEBUG("Connected, handle: %d", db->db_handle);

    return SUCCESS;
}

PHP_METHOD(Database, connect)
{
    zval *args = NULL;
    ISC_STATUS_ARRAY status = {0};

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(args, FireBird_Connect_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == database_connect(status, ZEND_THIS, args)) {
        update_err_props(status, FireBird_Database_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    object_init_ex(return_value, FireBird_Connection_ce);
    connection_ctor(return_value, ZEND_THIS);
    zend_update_property(FireBird_Connection_ce, O_SET(return_value, args));
}

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
int database_create(ISC_STATUS_ARRAY status, zval *db_o, zval *args_o)
{
    zval rv, *database;
    const char *dpb_buffer;
    short num_dpb_written;
    firebird_db *db = Z_DB_P(db_o);

    database = zend_read_property(FireBird_Create_Args_ce, O_GET(args_o, database), 0, &rv);

    if ((Z_TYPE_P(database) != IS_STRING) || !Z_STRLEN_P(database)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Database parameter not set");
        return FAILURE;
    }

    if (FAILURE == database_build_dpb(FireBird_Create_Args_ce, args_o, &database_create_zmap, &dpb_buffer, &num_dpb_written)) {
        return FAILURE;
    }

    FBDEBUG("database_create: %s", Z_STRVAL_P(database));
    if (isc_create_database(status, (short)Z_STRLEN_P(database), Z_STRVAL_P(database), &db->db_handle, num_dpb_written, dpb_buffer, 0)) {
        return FAILURE;
    }
    FBDEBUG("Created, handle: %d", db->db_handle);

    return SUCCESS;
}

PHP_METHOD(Database, create)
{
    zval *args = NULL;
    ISC_STATUS_ARRAY status = {0};

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(args, FireBird_Create_Args_ce)
    ZEND_PARSE_PARAMETERS_END();

    if (FAILURE == database_create(status, ZEND_THIS, args)) {
        update_err_props(status, FireBird_Database_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    object_init_ex(return_value, FireBird_Connection_ce);
    connection_ctor(return_value, ZEND_THIS);
    zend_update_property(FireBird_Connection_ce, O_SET(return_value, args));
}

PHP_METHOD(Database, drop)
{
    ZEND_PARSE_PARAMETERS_NONE();

    ISC_STATUS_ARRAY status = {0};
    firebird_db *db = Z_DB_P(ZEND_THIS);

    if (isc_drop_database(status, &db->db_handle)) {
        update_err_props(status, FireBird_Database_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

int database_get_info(ISC_STATUS_ARRAY status, isc_db_handle *db_handle, firebird_db_info *db_info,
    size_t info_req_size, char *info_req,
    size_t info_resp_size, char *info_resp,
    size_t max_limbo_count)
{
    if (isc_database_info(status, db_handle,
        info_req_size, info_req, info_resp_size, info_resp)) {
            return FAILURE;
    }

    // TODO: IXpbBuilder code duplication all over the place.
    // Maybe using firebird_xpb_zmap?
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* dpb;

    dpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_INFO_RESPONSE, info_resp, info_resp_size);

    ISC_INT64 len, total_len = 0;
    size_t limbo_count = 0;

#define READ_BIGINT(tag) \
    case isc_info_##tag: db_info->info_##tag = IXpbBuilder_getBigInt(dpb, st); break

    FBDEBUG("Parsing DB info buffer");
    for (IXpbBuilder_rewind(dpb, st); !IXpbBuilder_isEof(dpb, st); IXpbBuilder_moveNext(dpb, st)) {
        unsigned char tag = IXpbBuilder_getTag(dpb, st); total_len++;
        const ISC_UCHAR* b = IXpbBuilder_getBytes(dpb, st);
        len = IXpbBuilder_getLength(dpb, st); total_len += 2;
        total_len += len;

        switch(tag) {
            READ_BIGINT(db_id);
            READ_BIGINT(reads);
            READ_BIGINT(writes);
            READ_BIGINT(fetches);
            READ_BIGINT(marks);
            READ_BIGINT(page_size);
            READ_BIGINT(num_buffers);
            case isc_info_limbo: {
                if(limbo_count < max_limbo_count) {
                    db_info->info_limbo[limbo_count++] = IXpbBuilder_getBigInt(dpb, st);
                } else {
                    IXpbBuilder_moveNext(dpb, st);
                }
            } break;
            READ_BIGINT(current_memory);
            READ_BIGINT(max_memory);

            READ_BIGINT(allocation);
            READ_BIGINT(attachment_id);
            READ_BIGINT(read_seq_count);
            READ_BIGINT(read_idx_count);
            READ_BIGINT(insert_count);
            READ_BIGINT(update_count);
            READ_BIGINT(delete_count);
            READ_BIGINT(backout_count);
            READ_BIGINT(purge_count);
            READ_BIGINT(expunge_count);

            // All fields populated, not sure if needed
            // case fb_info_features: {
            //     for (unsigned i = 0; i < len; i++){
            //         if (b[i] == 0) {
            //             _php_firebird_module_error("Bad provider feature value: %d", b[i]);
            //         } else if (b[i] < fb_feature_max) {
            //             FBDEBUG_NOFL("  feature: %d", b[i]);
            //         }
            //     }
            // } break;

            case isc_info_end: break;
            case isc_info_truncated: {
                _php_firebird_module_error("DB info buffer error: truncated");
            } return FAILURE;
            case isc_info_error: {
                _php_firebird_module_error("DB info buffer error");
            } return FAILURE;

            default: {
                _php_firebird_module_fatal("BUG! Unhandled DB info tag: %d", tag);
            } break;
        }
    }

    db_info->info_limbo_count = limbo_count;

    return SUCCESS;
}

PHP_METHOD(Database, get_info)
{
    ZEND_PARSE_PARAMETERS_NONE();

    zval *args = NULL;
    ISC_STATUS_ARRAY status = { 0 };
    firebird_db *db = Z_DB_P(ZEND_THIS);

    // Full info, except limbo
    static char info_req[] = {
        isc_info_db_id,
        isc_info_reads,
        isc_info_writes,
        isc_info_fetches,
        isc_info_marks,

        // isc_info_implementation,
        // isc_info_isc_version / isc_info_firebird_version,
        // isc_info_base_level,
        isc_info_page_size,
        isc_info_num_buffers,
        // isc_info_limbo, // this will return a list, need parse into an array
        isc_info_current_memory,
        isc_info_max_memory,
        // isc_info_window_turns, / unused?
        // isc_info_license, / unused?

        isc_info_allocation,
        isc_info_attachment_id,

        // These are per table?
        isc_info_read_seq_count,
        isc_info_read_idx_count,
        isc_info_insert_count,
        isc_info_update_count,
        isc_info_delete_count,
        isc_info_backout_count,
        isc_info_purge_count,
        isc_info_expunge_count,

        // fb_info_features,

        isc_info_end
    };

    char info_resp[1024] = { 0 };

    if(database_get_info(status, &db->db_handle, &db->info, sizeof(info_req), info_req, sizeof(info_resp), info_resp, 0)) {
        update_err_props(status, FireBird_Database_ce, ZEND_THIS);
        RETURN_FALSE;
    }

    // const firebird_xpb_zmap *xpb_zmap = &database_info_zmap;

    // #define dd(name) db->info_##name
    // object_init_ex(return_value, FireBird_Db_Info_ce);
    // for (int i = 0; i < xpb_zmap->count; i++) {
    //     switch(xpb_zmap->ztypes[i]){
    //         case MAY_BE_LONG: {
    //             zend_update_property_long(FireBird_Db_Info_ce, Z_OBJ_P(return_value),
    //                 xpb_zmap->names[i], sizeof(xpb_zmap->names[i]) - 1, dd(xpb_zmap->names[i]));
    //         } break;

    //         default: {
    //             _php_firebird_module_error("BUG: unhandled firebird_xpb_zmap entry: %s with type: %d", xpb_zmap->names[i], xpb_zmap->ztypes[i]);
    //         } break;
    //     }
    // }

    object_init_ex(return_value, FireBird_Db_Info_ce);

#define UP_LONG(name) \
    zend_update_property_long(FireBird_Db_Info_ce, Z_OBJ_P(return_value), #name, sizeof(#name) - 1, db->info.info_##name)

    UP_LONG(db_id);
    UP_LONG(reads);
    UP_LONG(writes);
    UP_LONG(fetches);
    UP_LONG(marks);

    UP_LONG(page_size);
    UP_LONG(num_buffers);
    UP_LONG(current_memory);
    UP_LONG(max_memory);

    UP_LONG(allocation);
    UP_LONG(attachment_id);
    UP_LONG(read_seq_count);
    UP_LONG(read_idx_count);
    UP_LONG(insert_count);
    UP_LONG(update_count);
    UP_LONG(delete_count);
    UP_LONG(backout_count);
    UP_LONG(purge_count);
    UP_LONG(expunge_count);
}

PHP_METHOD(Database, on_event)
{
    ISC_STATUS_ARRAY status;

    char *name;
    size_t name_len;

    firebird_db *db = Z_DB_P(ZEND_THIS);

    zval ievent;
    object_init_ex(&ievent, FireBird_Event_ce);

    firebird_event *event = Z_EVENT_P(&ievent);
    ZVAL_COPY(&event->instance, &ievent);

    // zval *f;
    // zend_fcall_info fci;
    // zend_fcall_info_cache fcc;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(name, name_len)
        // Z_PARAM_OBJECT_OF_CLASS(f, zend_ce_fiber)
        // Z_PARAM_FUNC_NO_TRAMPOLINE_FREE(event->fci, event->fcc)
        Z_PARAM_FUNC(event->fci, event->fcc)
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

    if (isc_que_events(status, event->db_handle, &event->event_id, event->buff_len, event->event_buffer, event_ast_routine, NULL)) {
        update_err_props(status, FireBird_Database_ce, ZEND_THIS);
        zval_ptr_dtor(&ievent);
        RETURN_FALSE;
    }

    fb_events.events = event;
    fb_events.count++;

    RETURN_TRUE;
}

const zend_function_entry FireBird_Database_methods[] = {
    PHP_ME(Database, connect, arginfo_FireBird_Database_connect, ZEND_ACC_PUBLIC)
    PHP_ME(Database, create, arginfo_FireBird_Database_create, ZEND_ACC_PUBLIC)
    PHP_ME(Database, drop, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Database, get_info, arginfo_FireBird_Database_get_info, ZEND_ACC_PUBLIC)
    PHP_ME(Database, on_event, arginfo_FireBird_Database_on_event, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object *FireBird_Database_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Database_create");

    firebird_db *db = zend_object_alloc(sizeof(firebird_db), ce);
    db->db_handle = 0;

    zend_object_std_init(&db->std, ce);
    object_properties_init(&db->std, ce);

    return &db->std;
}

static void FireBird_Database_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Database_free_obj");

    firebird_db *db = Z_DB_O(obj);

    if(db->db_handle) {
        ISC_STATUS_ARRAY status;
        if(isc_detach_database(status, &db->db_handle)) {
            status_fbp_error(status);
        } else {
            db->db_handle = 0;
        }
    }

    zend_object_std_dtor(&db->std);
}

void register_FireBird_Database_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Database", FireBird_Database_methods);
    FireBird_Database_ce = zend_register_internal_class(&tmp_ce);

    DECLARE_ERR_PROPS(FireBird_Database_ce);

    zend_class_implements(FireBird_Database_ce, 1, FireBird_IError_ce);

    FireBird_Database_ce->create_object = FireBird_Database_create;
    FireBird_Database_ce->default_object_handlers = &FireBird_Database_object_handlers;

    memcpy(&FireBird_Database_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Database_object_handlers.offset = XtOffsetOf(firebird_db, std);
    FireBird_Database_object_handlers.free_obj = FireBird_Database_free_obj;
}

int database_build_dpb(zend_class_entry *ce, zval *args_o, const firebird_xpb_zmap *xpb_zmap, const char **dpb_buf, short *num_dpb_written)
{
    struct IMaster* master = fb_get_master_interface();
    struct IStatus* st = IMaster_getStatus(master);
    struct IUtil* utl = IMaster_getUtilInterface(master);
    struct IXpbBuilder* xpb = IUtil_getXpbBuilder(utl, st, IXpbBuilder_DPB, NULL, 0);

    zend_property_info *prop_info = NULL;
    zend_string *prop_name = NULL;
    zval rv, *val, *checkval;
    int i;

    xpb_insert_tag(isc_dpb_version2);
    xpb_insert_int(isc_dpb_sql_dialect, SQL_DIALECT_CURRENT);
    for (int i = 0; i < xpb_zmap->count; i++) {
        prop_name = zend_string_init(xpb_zmap->names[i], strlen(xpb_zmap->names[i]), 1);

#ifdef PHP_DEBUG
        if (!zend_hash_exists(&ce->properties_info, prop_name)) {
            _php_firebird_module_fatal("BUG! Property %s does not exist for %s::%s. Verify xpb_zmap",
                xpb_zmap->names[i], ZSTR_VAL(ce->name), xpb_zmap->names[i]);
            zend_string_release(prop_name);
            continue;
        }
#endif

        prop_info = zend_get_property_info(ce, prop_name, 0);
        checkval = OBJ_PROP(Z_OBJ_P(args_o), prop_info->offset);
        if (Z_ISUNDEF_P(checkval)) {
            FBDEBUG("property: %s is uninitialized", xpb_zmap->names[i]);
            zend_string_release(prop_name);
            continue;
        }

        val = zend_read_property_ex(ce, Z_OBJ_P(args_o), prop_name, 0, &rv);
        zend_string_release(prop_name);

        switch (Z_TYPE_P(val)) {
            case IS_STRING:
                FBDEBUG("property: %s is string: `%s`", xpb_zmap->names[i], Z_STRVAL_P(val));
                xpb_insert_string(xpb_zmap->tags[i], Z_STRVAL_P(val));
                break;
            case IS_LONG:
                FBDEBUG("property: %s is long: `%u`", xpb_zmap->names[i], Z_LVAL_P(val));
                xpb_insert_int(xpb_zmap->tags[i], (int)Z_LVAL_P(val));
                break;
            case IS_TRUE:
                FBDEBUG("property: %s is true", xpb_zmap->names[i]);
                xpb_insert_true(xpb_zmap->tags[i]);
                break;
            case IS_FALSE:
                FBDEBUG("property: %s is false", xpb_zmap->names[i]);
                xpb_insert_false(xpb_zmap->tags[i]);
                break;
            case IS_NULL:
                FBDEBUG("property: %s is null", xpb_zmap->names[i]);
                break;
            default:
                _php_firebird_module_fatal("BUG! Unhandled: type %s for property %s::%s",
                    zend_get_type_by_const(Z_TYPE_P(val)), ZSTR_VAL(ce->name), xpb_zmap->names[i]);
                break;
        }
    }

#if FB_API_VER >= 40
    // Do not handle directly INT128 or DECFLOAT, convert to VARCHAR at server instead
    xpb_insert_string(isc_dpb_set_bind, "int128 to varchar;decfloat to varchar");
#endif

    *num_dpb_written = IXpbBuilder_getBufferLength(xpb, st);
    *dpb_buf = IXpbBuilder_getBuffer(xpb, st);

    // Needed? Not needed?
    // IXpbBuilder_dispose(xpb);
    // IStatus_dispose(st);

    return SUCCESS;
}

void register_FireBird_Db_Info_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Db_Info", NULL);
    FireBird_Db_Info_ce = zend_register_internal_class(&tmp_ce);

    declare_props_zmap(FireBird_Db_Info_ce, &database_info_zmap);
}
