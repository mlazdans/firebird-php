/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 290b2a759626133306021f6bd58b4fb438599f3a */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_FireBird_set_error_handler, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, handler, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_FireBird_Database___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Database_connect, 0, 1, FireBird\\Database, MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, args, FireBird\\Connect_Args, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Database_create, 0, 1, FireBird\\Database, MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, args, FireBird\\Create_Args, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Database_start_transaction, 0, 0, FireBird\\Transaction, MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, tb, FireBird\\TBuilder, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FireBird_Database_new_transaction, 0, 0, FireBird\\Transaction, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Database_get_info, 0, 0, FireBird\\Db_Info, MAY_BE_FALSE)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Database_disconnect, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Database_drop arginfo_class_FireBird_Database_disconnect

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Database_on_event, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, f, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Database_reconnect_transaction, 0, 1, FireBird\\Transaction, MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, id, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_FireBird_Database_get_limbo_transactions, 0, 1, MAY_BE_ARRAY|MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, max_count, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Multi_Transaction___construct arginfo_class_FireBird_Database___construct

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FireBird_Multi_Transaction_add_db, 0, 1, FireBird\\Transaction, 0)
	ZEND_ARG_OBJ_INFO(0, database, FireBird\\Database, 0)
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, builder, FireBird\\TBuilder, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Multi_Transaction_start arginfo_class_FireBird_Database_disconnect

#define arginfo_class_FireBird_Multi_Transaction_commit arginfo_class_FireBird_Database_disconnect

#define arginfo_class_FireBird_Multi_Transaction_commit_ret arginfo_class_FireBird_Database_disconnect

#define arginfo_class_FireBird_Multi_Transaction_rollback arginfo_class_FireBird_Database_disconnect

#define arginfo_class_FireBird_Multi_Transaction_rollback_ret arginfo_class_FireBird_Database_disconnect

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Multi_Transaction_prepare_2pc, 0, 0, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, description, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_FireBird_Transaction___construct, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, database, FireBird\\Database, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Transaction_start, 0, 0, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, builder, FireBird\\TBuilder, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Transaction_commit arginfo_class_FireBird_Database_disconnect

#define arginfo_class_FireBird_Transaction_commit_ret arginfo_class_FireBird_Database_disconnect

#define arginfo_class_FireBird_Transaction_rollback arginfo_class_FireBird_Database_disconnect

#define arginfo_class_FireBird_Transaction_rollback_ret arginfo_class_FireBird_Database_disconnect

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Transaction_query, 0, 1, FireBird\\Statement, MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, sql, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, bind_args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Transaction_prepare, 0, 1, FireBird\\Statement, MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, sql, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Transaction_open_blob, 0, 1, FireBird\\Blob, MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, id, FireBird\\Blob_Id, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Transaction_create_blob, 0, 0, FireBird\\Blob, MAY_BE_FALSE)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Transaction_execute, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, sql, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Statement___construct arginfo_class_FireBird_Database___construct

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_FireBird_Statement_fetch_object, 0, 0, MAY_BE_OBJECT|MAY_BE_FALSE|MAY_BE_NULL)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_FireBird_Statement_fetch_array, 0, 0, MAY_BE_ARRAY|MAY_BE_FALSE|MAY_BE_NULL)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Statement_fetch_row arginfo_class_FireBird_Statement_fetch_array

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Statement_execute, 0, 0, _IS_BOOL, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, bind_args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Statement_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Statement_get_var_info_in, 0, 1, FireBird\\Var_Info, MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, num, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Statement_get_var_info_out arginfo_class_FireBird_Statement_get_var_info_in

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Statement_set_name, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Blob___construct arginfo_class_FireBird_Database___construct

#define arginfo_class_FireBird_Blob_close arginfo_class_FireBird_Database_disconnect

#define arginfo_class_FireBird_Blob_cancel arginfo_class_FireBird_Database_disconnect

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_FireBird_Blob_get, 0, 0, MAY_BE_STRING|MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, max_len, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Blob_put, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_FireBird_Blob_seek, 0, 2, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, pos, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, mode, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Event___construct arginfo_class_FireBird_Database___construct

#define arginfo_class_FireBird_Event_consume arginfo_class_FireBird_Database_disconnect

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_connect, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, args, FireBird\\Service_Connect_Args, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Service_disconnect arginfo_class_FireBird_Database_disconnect

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Service_get_server_info, 0, 0, FireBird\\Server_Info, MAY_BE_FALSE)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_add_user, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, user_info, FireBird\\Server_User_Info, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Service_modify_user arginfo_class_FireBird_Service_add_user

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_delete_user, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, username, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_backup, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, bkp_file, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_restore, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, bkp_file, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_shutdown_db, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, timeout, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_db_online, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_set_page_buffers, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, buffers, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_set_sweep_interval, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, interval, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_deny_new_attachments, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Service_deny_new_transactions arginfo_class_FireBird_Service_deny_new_attachments

#define arginfo_class_FireBird_Service_set_write_mode_async arginfo_class_FireBird_Service_deny_new_attachments

#define arginfo_class_FireBird_Service_set_write_mode_sync arginfo_class_FireBird_Service_deny_new_attachments

#define arginfo_class_FireBird_Service_set_access_mode_readonly arginfo_class_FireBird_Service_deny_new_attachments

#define arginfo_class_FireBird_Service_set_access_mode_readwrite arginfo_class_FireBird_Service_deny_new_attachments

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_enable_reserve_space, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, database, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_Service_disable_reserve_space arginfo_class_FireBird_Service_enable_reserve_space

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Service_set_sql_dialect, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, database, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dialect, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FireBird_TBuilder_read_only, 0, 0, FireBird\\TBuilder, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, enable, _IS_BOOL, 0, "true")
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_TBuilder_ignore_limbo arginfo_class_FireBird_TBuilder_read_only

#define arginfo_class_FireBird_TBuilder_auto_commit arginfo_class_FireBird_TBuilder_read_only

#define arginfo_class_FireBird_TBuilder_no_auto_undo arginfo_class_FireBird_TBuilder_read_only

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FireBird_TBuilder_wait, 0, 0, FireBird\\TBuilder, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, lock_timeout, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FireBird_TBuilder_isolation_snapshot, 0, 0, FireBird\\TBuilder, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, at_number, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FireBird_TBuilder_isolation_snapshot_table_stability, 0, 0, FireBird\\TBuilder, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FireBird_TBuilder_isolation_read_committed_record_version arginfo_class_FireBird_TBuilder_isolation_snapshot_table_stability

#define arginfo_class_FireBird_TBuilder_isolation_read_committed_no_record_version arginfo_class_FireBird_TBuilder_isolation_snapshot_table_stability

#define arginfo_class_FireBird_TBuilder_isolation_read_committed_read_consistency arginfo_class_FireBird_TBuilder_isolation_snapshot_table_stability

#define arginfo_class_FireBird_TBuilder_dump_state arginfo_class_FireBird_Statement_free

#define arginfo_class_FireBird_Blob_Id___construct arginfo_class_FireBird_Database___construct

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FireBird_Blob_Id_to_legacy_id, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_FireBird_Blob_Id_from_legacy_id, 0, 1, FireBird\\Blob_Id, MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, legacy_id, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_FUNCTION(FireBird_set_error_handler);
ZEND_METHOD(FireBird_Database, __construct);
ZEND_METHOD(FireBird_Database, connect);
ZEND_METHOD(FireBird_Database, create);
ZEND_METHOD(FireBird_Database, start_transaction);
ZEND_METHOD(FireBird_Database, new_transaction);
ZEND_METHOD(FireBird_Database, get_info);
ZEND_METHOD(FireBird_Database, disconnect);
ZEND_METHOD(FireBird_Database, drop);
ZEND_METHOD(FireBird_Database, on_event);
ZEND_METHOD(FireBird_Database, reconnect_transaction);
ZEND_METHOD(FireBird_Database, get_limbo_transactions);
ZEND_METHOD(FireBird_Multi_Transaction, __construct);
ZEND_METHOD(FireBird_Multi_Transaction, add_db);
ZEND_METHOD(FireBird_Multi_Transaction, start);
ZEND_METHOD(FireBird_Multi_Transaction, commit);
ZEND_METHOD(FireBird_Multi_Transaction, commit_ret);
ZEND_METHOD(FireBird_Multi_Transaction, rollback);
ZEND_METHOD(FireBird_Multi_Transaction, rollback_ret);
ZEND_METHOD(FireBird_Multi_Transaction, prepare_2pc);
ZEND_METHOD(FireBird_Transaction, __construct);
ZEND_METHOD(FireBird_Transaction, start);
ZEND_METHOD(FireBird_Transaction, commit);
ZEND_METHOD(FireBird_Transaction, commit_ret);
ZEND_METHOD(FireBird_Transaction, rollback);
ZEND_METHOD(FireBird_Transaction, rollback_ret);
ZEND_METHOD(FireBird_Transaction, query);
ZEND_METHOD(FireBird_Transaction, prepare);
ZEND_METHOD(FireBird_Transaction, open_blob);
ZEND_METHOD(FireBird_Transaction, create_blob);
ZEND_METHOD(FireBird_Transaction, execute);
ZEND_METHOD(FireBird_Statement, __construct);
ZEND_METHOD(FireBird_Statement, fetch_object);
ZEND_METHOD(FireBird_Statement, fetch_array);
ZEND_METHOD(FireBird_Statement, fetch_row);
ZEND_METHOD(FireBird_Statement, execute);
ZEND_METHOD(FireBird_Statement, free);
ZEND_METHOD(FireBird_Statement, get_var_info_in);
ZEND_METHOD(FireBird_Statement, get_var_info_out);
ZEND_METHOD(FireBird_Statement, set_name);
ZEND_METHOD(FireBird_Blob, __construct);
ZEND_METHOD(FireBird_Blob, close);
ZEND_METHOD(FireBird_Blob, cancel);
ZEND_METHOD(FireBird_Blob, get);
ZEND_METHOD(FireBird_Blob, put);
ZEND_METHOD(FireBird_Blob, seek);
ZEND_METHOD(FireBird_Event, __construct);
ZEND_METHOD(FireBird_Event, consume);
ZEND_METHOD(FireBird_Service, connect);
ZEND_METHOD(FireBird_Service, disconnect);
ZEND_METHOD(FireBird_Service, get_server_info);
ZEND_METHOD(FireBird_Service, add_user);
ZEND_METHOD(FireBird_Service, modify_user);
ZEND_METHOD(FireBird_Service, delete_user);
ZEND_METHOD(FireBird_Service, backup);
ZEND_METHOD(FireBird_Service, restore);
ZEND_METHOD(FireBird_Service, shutdown_db);
ZEND_METHOD(FireBird_Service, db_online);
ZEND_METHOD(FireBird_Service, set_page_buffers);
ZEND_METHOD(FireBird_Service, set_sweep_interval);
ZEND_METHOD(FireBird_Service, deny_new_attachments);
ZEND_METHOD(FireBird_Service, deny_new_transactions);
ZEND_METHOD(FireBird_Service, set_write_mode_async);
ZEND_METHOD(FireBird_Service, set_write_mode_sync);
ZEND_METHOD(FireBird_Service, set_access_mode_readonly);
ZEND_METHOD(FireBird_Service, set_access_mode_readwrite);
ZEND_METHOD(FireBird_Service, enable_reserve_space);
ZEND_METHOD(FireBird_Service, disable_reserve_space);
ZEND_METHOD(FireBird_Service, set_sql_dialect);
ZEND_METHOD(FireBird_TBuilder, read_only);
ZEND_METHOD(FireBird_TBuilder, ignore_limbo);
ZEND_METHOD(FireBird_TBuilder, auto_commit);
ZEND_METHOD(FireBird_TBuilder, no_auto_undo);
ZEND_METHOD(FireBird_TBuilder, wait);
ZEND_METHOD(FireBird_TBuilder, isolation_snapshot);
ZEND_METHOD(FireBird_TBuilder, isolation_snapshot_table_stability);
ZEND_METHOD(FireBird_TBuilder, isolation_read_committed_record_version);
ZEND_METHOD(FireBird_TBuilder, isolation_read_committed_no_record_version);
ZEND_METHOD(FireBird_TBuilder, isolation_read_committed_read_consistency);
ZEND_METHOD(FireBird_TBuilder, dump_state);
ZEND_METHOD(FireBird_Blob_Id, __construct);
ZEND_METHOD(FireBird_Blob_Id, to_legacy_id);
ZEND_METHOD(FireBird_Blob_Id, from_legacy_id);

static const zend_function_entry ext_functions[] = {
	ZEND_RAW_FENTRY(ZEND_NS_NAME("FireBird", "set_error_handler"), zif_FireBird_set_error_handler, arginfo_FireBird_set_error_handler, 0, NULL, NULL)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_Database_methods[] = {
	ZEND_ME(FireBird_Database, __construct, arginfo_class_FireBird_Database___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(FireBird_Database, connect, arginfo_class_FireBird_Database_connect, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(FireBird_Database, create, arginfo_class_FireBird_Database_create, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(FireBird_Database, start_transaction, arginfo_class_FireBird_Database_start_transaction, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Database, new_transaction, arginfo_class_FireBird_Database_new_transaction, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Database, get_info, arginfo_class_FireBird_Database_get_info, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Database, disconnect, arginfo_class_FireBird_Database_disconnect, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Database, drop, arginfo_class_FireBird_Database_drop, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Database, on_event, arginfo_class_FireBird_Database_on_event, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Database, reconnect_transaction, arginfo_class_FireBird_Database_reconnect_transaction, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Database, get_limbo_transactions, arginfo_class_FireBird_Database_get_limbo_transactions, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_Multi_Transaction_methods[] = {
	ZEND_ME(FireBird_Multi_Transaction, __construct, arginfo_class_FireBird_Multi_Transaction___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Multi_Transaction, add_db, arginfo_class_FireBird_Multi_Transaction_add_db, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Multi_Transaction, start, arginfo_class_FireBird_Multi_Transaction_start, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Multi_Transaction, commit, arginfo_class_FireBird_Multi_Transaction_commit, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Multi_Transaction, commit_ret, arginfo_class_FireBird_Multi_Transaction_commit_ret, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Multi_Transaction, rollback, arginfo_class_FireBird_Multi_Transaction_rollback, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Multi_Transaction, rollback_ret, arginfo_class_FireBird_Multi_Transaction_rollback_ret, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Multi_Transaction, prepare_2pc, arginfo_class_FireBird_Multi_Transaction_prepare_2pc, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_Transaction_methods[] = {
	ZEND_ME(FireBird_Transaction, __construct, arginfo_class_FireBird_Transaction___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, start, arginfo_class_FireBird_Transaction_start, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, commit, arginfo_class_FireBird_Transaction_commit, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, commit_ret, arginfo_class_FireBird_Transaction_commit_ret, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, rollback, arginfo_class_FireBird_Transaction_rollback, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, rollback_ret, arginfo_class_FireBird_Transaction_rollback_ret, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, query, arginfo_class_FireBird_Transaction_query, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, prepare, arginfo_class_FireBird_Transaction_prepare, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, open_blob, arginfo_class_FireBird_Transaction_open_blob, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, create_blob, arginfo_class_FireBird_Transaction_create_blob, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Transaction, execute, arginfo_class_FireBird_Transaction_execute, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_Statement_methods[] = {
	ZEND_ME(FireBird_Statement, __construct, arginfo_class_FireBird_Statement___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(FireBird_Statement, fetch_object, arginfo_class_FireBird_Statement_fetch_object, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Statement, fetch_array, arginfo_class_FireBird_Statement_fetch_array, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Statement, fetch_row, arginfo_class_FireBird_Statement_fetch_row, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Statement, execute, arginfo_class_FireBird_Statement_execute, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Statement, free, arginfo_class_FireBird_Statement_free, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Statement, get_var_info_in, arginfo_class_FireBird_Statement_get_var_info_in, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Statement, get_var_info_out, arginfo_class_FireBird_Statement_get_var_info_out, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Statement, set_name, arginfo_class_FireBird_Statement_set_name, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_Blob_methods[] = {
	ZEND_ME(FireBird_Blob, __construct, arginfo_class_FireBird_Blob___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(FireBird_Blob, close, arginfo_class_FireBird_Blob_close, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Blob, cancel, arginfo_class_FireBird_Blob_cancel, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Blob, get, arginfo_class_FireBird_Blob_get, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Blob, put, arginfo_class_FireBird_Blob_put, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Blob, seek, arginfo_class_FireBird_Blob_seek, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_Event_methods[] = {
	ZEND_ME(FireBird_Event, __construct, arginfo_class_FireBird_Event___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(FireBird_Event, consume, arginfo_class_FireBird_Event_consume, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_Service_methods[] = {
	ZEND_ME(FireBird_Service, connect, arginfo_class_FireBird_Service_connect, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, disconnect, arginfo_class_FireBird_Service_disconnect, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, get_server_info, arginfo_class_FireBird_Service_get_server_info, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, add_user, arginfo_class_FireBird_Service_add_user, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, modify_user, arginfo_class_FireBird_Service_modify_user, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, delete_user, arginfo_class_FireBird_Service_delete_user, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, backup, arginfo_class_FireBird_Service_backup, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, restore, arginfo_class_FireBird_Service_restore, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, shutdown_db, arginfo_class_FireBird_Service_shutdown_db, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, db_online, arginfo_class_FireBird_Service_db_online, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, set_page_buffers, arginfo_class_FireBird_Service_set_page_buffers, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, set_sweep_interval, arginfo_class_FireBird_Service_set_sweep_interval, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, deny_new_attachments, arginfo_class_FireBird_Service_deny_new_attachments, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, deny_new_transactions, arginfo_class_FireBird_Service_deny_new_transactions, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, set_write_mode_async, arginfo_class_FireBird_Service_set_write_mode_async, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, set_write_mode_sync, arginfo_class_FireBird_Service_set_write_mode_sync, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, set_access_mode_readonly, arginfo_class_FireBird_Service_set_access_mode_readonly, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, set_access_mode_readwrite, arginfo_class_FireBird_Service_set_access_mode_readwrite, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, enable_reserve_space, arginfo_class_FireBird_Service_enable_reserve_space, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, disable_reserve_space, arginfo_class_FireBird_Service_disable_reserve_space, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_Service, set_sql_dialect, arginfo_class_FireBird_Service_set_sql_dialect, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_TBuilder_methods[] = {
	ZEND_ME(FireBird_TBuilder, read_only, arginfo_class_FireBird_TBuilder_read_only, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, ignore_limbo, arginfo_class_FireBird_TBuilder_ignore_limbo, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, auto_commit, arginfo_class_FireBird_TBuilder_auto_commit, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, no_auto_undo, arginfo_class_FireBird_TBuilder_no_auto_undo, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, wait, arginfo_class_FireBird_TBuilder_wait, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, isolation_snapshot, arginfo_class_FireBird_TBuilder_isolation_snapshot, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, isolation_snapshot_table_stability, arginfo_class_FireBird_TBuilder_isolation_snapshot_table_stability, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, isolation_read_committed_record_version, arginfo_class_FireBird_TBuilder_isolation_read_committed_record_version, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, isolation_read_committed_no_record_version, arginfo_class_FireBird_TBuilder_isolation_read_committed_no_record_version, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, isolation_read_committed_read_consistency, arginfo_class_FireBird_TBuilder_isolation_read_committed_read_consistency, ZEND_ACC_PUBLIC)
	ZEND_ME(FireBird_TBuilder, dump_state, arginfo_class_FireBird_TBuilder_dump_state, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FireBird_Blob_Id_methods[] = {
	ZEND_ME(FireBird_Blob_Id, __construct, arginfo_class_FireBird_Blob_Id___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(FireBird_Blob_Id, to_legacy_id, arginfo_class_FireBird_Blob_Id_to_legacy_id, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(FireBird_Blob_Id, from_legacy_id, arginfo_class_FireBird_Blob_Id_from_legacy_id, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};

static void register_firebird_symbols(int module_number)
{
	REGISTER_LONG_CONSTANT("FireBird\\FETCH_BLOB_TEXT", FBP_FETCH_BLOB_TEXT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\FETCH_UNIXTIME", FBP_FETCH_UNIXTIME, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\FETCH_DATE_OBJ", FBP_FETCH_DATE_OBJ, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\FETCH_FETCH_BLOB_TEXT", FBP_FETCH_BLOB_TEXT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\BLOB_TYPE_SEGMENTED", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\BLOB_TYPE_STREAMED", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\BLOB_SEEK_START", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\BLOB_SEEK_CURRENT", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\BLOB_SEEK_END", 2, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\SM_NORMAL", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\SM_MULTI", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\SM_SINGLE", 2, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FireBird\\SM_FULL", 3, CONST_PERSISTENT);
}

static zend_class_entry *register_class_FireBird_Database(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Database", class_FireBird_Database_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_args_default_value;
	ZVAL_UNDEF(&property_args_default_value);
	zend_string *property_args_class_FireBird_Connect_Args = zend_string_init("FireBird\\Connect_Args", sizeof("FireBird\\Connect_Args") - 1, 1);
	zend_string *property_args_class_FireBird_Create_Args = zend_string_init("FireBird\\Create_Args", sizeof("FireBird\\Create_Args") - 1, 1);
	zend_type_list *property_args_type_list = malloc(ZEND_TYPE_LIST_SIZE(2));
	property_args_type_list->num_types = 2;
	property_args_type_list->types[0] = (zend_type) ZEND_TYPE_INIT_CLASS(property_args_class_FireBird_Connect_Args, 0, 0);
	property_args_type_list->types[1] = (zend_type) ZEND_TYPE_INIT_CLASS(property_args_class_FireBird_Create_Args, 0, 0);
	zend_type property_args_type = ZEND_TYPE_INIT_UNION(property_args_type_list, 0);
	zend_declare_typed_property(class_entry, ZSTR_KNOWN(ZEND_STR_ARGS), &property_args_default_value, ZEND_ACC_PROTECTED, NULL, property_args_type);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Multi_Transaction(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Multi_Transaction", class_FireBird_Multi_Transaction_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Transaction(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Transaction", class_FireBird_Transaction_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_id_default_value;
	ZVAL_UNDEF(&property_id_default_value);
	zend_string *property_id_name = zend_string_init("id", sizeof("id") - 1, true);
	zend_declare_typed_property(class_entry, property_id_name, &property_id_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_id_name, true);

	zval property_database_default_value;
	ZVAL_UNDEF(&property_database_default_value);
	zend_string *property_database_name = zend_string_init("database", sizeof("database") - 1, true);
	zend_string *property_database_class_FireBird_Database = zend_string_init("FireBird\\Database", sizeof("FireBird\\Database")-1, 1);
	zend_declare_typed_property(class_entry, property_database_name, &property_database_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_CLASS(property_database_class_FireBird_Database, 0, 0));
	zend_string_release_ex(property_database_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Statement(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Statement", class_FireBird_Statement_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_transaction_default_value;
	ZVAL_UNDEF(&property_transaction_default_value);
	zend_string *property_transaction_name = zend_string_init("transaction", sizeof("transaction") - 1, true);
	zend_string *property_transaction_class_FireBird_Transaction = zend_string_init("FireBird\\Transaction", sizeof("FireBird\\Transaction")-1, 1);
	zend_declare_typed_property(class_entry, property_transaction_name, &property_transaction_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_CLASS(property_transaction_class_FireBird_Transaction, 0, 0));
	zend_string_release_ex(property_transaction_name, true);

	zval property_name_default_value;
	ZVAL_UNDEF(&property_name_default_value);
	zend_declare_typed_property(class_entry, ZSTR_KNOWN(ZEND_STR_NAME), &property_name_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));

	zval property_in_vars_count_default_value;
	ZVAL_UNDEF(&property_in_vars_count_default_value);
	zend_string *property_in_vars_count_name = zend_string_init("in_vars_count", sizeof("in_vars_count") - 1, true);
	zend_declare_typed_property(class_entry, property_in_vars_count_name, &property_in_vars_count_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_in_vars_count_name, true);

	zval property_out_vars_count_default_value;
	ZVAL_UNDEF(&property_out_vars_count_default_value);
	zend_string *property_out_vars_count_name = zend_string_init("out_vars_count", sizeof("out_vars_count") - 1, true);
	zend_declare_typed_property(class_entry, property_out_vars_count_name, &property_out_vars_count_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_out_vars_count_name, true);

	zval property_insert_count_default_value;
	ZVAL_UNDEF(&property_insert_count_default_value);
	zend_string *property_insert_count_name = zend_string_init("insert_count", sizeof("insert_count") - 1, true);
	zend_declare_typed_property(class_entry, property_insert_count_name, &property_insert_count_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_insert_count_name, true);

	zval property_update_count_default_value;
	ZVAL_UNDEF(&property_update_count_default_value);
	zend_string *property_update_count_name = zend_string_init("update_count", sizeof("update_count") - 1, true);
	zend_declare_typed_property(class_entry, property_update_count_name, &property_update_count_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_update_count_name, true);

	zval property_delete_count_default_value;
	ZVAL_UNDEF(&property_delete_count_default_value);
	zend_string *property_delete_count_name = zend_string_init("delete_count", sizeof("delete_count") - 1, true);
	zend_declare_typed_property(class_entry, property_delete_count_name, &property_delete_count_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_delete_count_name, true);

	zval property_affected_count_default_value;
	ZVAL_UNDEF(&property_affected_count_default_value);
	zend_string *property_affected_count_name = zend_string_init("affected_count", sizeof("affected_count") - 1, true);
	zend_declare_typed_property(class_entry, property_affected_count_name, &property_affected_count_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_affected_count_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Blob(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Blob", class_FireBird_Blob_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_num_segments_default_value;
	ZVAL_UNDEF(&property_num_segments_default_value);
	zend_string *property_num_segments_name = zend_string_init("num_segments", sizeof("num_segments") - 1, true);
	zend_declare_typed_property(class_entry, property_num_segments_name, &property_num_segments_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_num_segments_name, true);

	zval property_max_segment_default_value;
	ZVAL_UNDEF(&property_max_segment_default_value);
	zend_string *property_max_segment_name = zend_string_init("max_segment", sizeof("max_segment") - 1, true);
	zend_declare_typed_property(class_entry, property_max_segment_name, &property_max_segment_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_max_segment_name, true);

	zval property_total_length_default_value;
	ZVAL_UNDEF(&property_total_length_default_value);
	zend_string *property_total_length_name = zend_string_init("total_length", sizeof("total_length") - 1, true);
	zend_declare_typed_property(class_entry, property_total_length_name, &property_total_length_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_total_length_name, true);

	zval property_type_default_value;
	ZVAL_UNDEF(&property_type_default_value);
	zend_declare_typed_property(class_entry, ZSTR_KNOWN(ZEND_STR_TYPE), &property_type_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));

	zval property_position_default_value;
	ZVAL_UNDEF(&property_position_default_value);
	zend_string *property_position_name = zend_string_init("position", sizeof("position") - 1, true);
	zend_declare_typed_property(class_entry, property_position_name, &property_position_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_position_name, true);

	zval property_is_writable_default_value;
	ZVAL_UNDEF(&property_is_writable_default_value);
	zend_string *property_is_writable_name = zend_string_init("is_writable", sizeof("is_writable") - 1, true);
	zend_declare_typed_property(class_entry, property_is_writable_name, &property_is_writable_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_is_writable_name, true);

	zval property_transaction_default_value;
	ZVAL_UNDEF(&property_transaction_default_value);
	zend_string *property_transaction_name = zend_string_init("transaction", sizeof("transaction") - 1, true);
	zend_string *property_transaction_class_FireBird_Transaction = zend_string_init("FireBird\\Transaction", sizeof("FireBird\\Transaction")-1, 1);
	zend_declare_typed_property(class_entry, property_transaction_name, &property_transaction_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_CLASS(property_transaction_class_FireBird_Transaction, 0, 0));
	zend_string_release_ex(property_transaction_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Event(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Event", class_FireBird_Event_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Service(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Service", class_FireBird_Service_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_args_default_value;
	ZVAL_UNDEF(&property_args_default_value);
	zend_string *property_args_class_FireBird_Service_Connect_Args = zend_string_init("FireBird\\Service_Connect_Args", sizeof("FireBird\\Service_Connect_Args")-1, 1);
	zend_declare_typed_property(class_entry, ZSTR_KNOWN(ZEND_STR_ARGS), &property_args_default_value, ZEND_ACC_PROTECTED, NULL, (zend_type) ZEND_TYPE_INIT_CLASS(property_args_class_FireBird_Service_Connect_Args, 0, 0));

	return class_entry;
}

static zend_class_entry *register_class_FireBird_TBuilder(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "TBuilder", class_FireBird_TBuilder_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_is_read_only_default_value;
	ZVAL_UNDEF(&property_is_read_only_default_value);
	zend_string *property_is_read_only_name = zend_string_init("is_read_only", sizeof("is_read_only") - 1, true);
	zend_declare_typed_property(class_entry, property_is_read_only_name, &property_is_read_only_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_is_read_only_name, true);

	zval property_is_ignore_limbo_default_value;
	ZVAL_UNDEF(&property_is_ignore_limbo_default_value);
	zend_string *property_is_ignore_limbo_name = zend_string_init("is_ignore_limbo", sizeof("is_ignore_limbo") - 1, true);
	zend_declare_typed_property(class_entry, property_is_ignore_limbo_name, &property_is_ignore_limbo_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_is_ignore_limbo_name, true);

	zval property_is_auto_commit_default_value;
	ZVAL_UNDEF(&property_is_auto_commit_default_value);
	zend_string *property_is_auto_commit_name = zend_string_init("is_auto_commit", sizeof("is_auto_commit") - 1, true);
	zend_declare_typed_property(class_entry, property_is_auto_commit_name, &property_is_auto_commit_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_is_auto_commit_name, true);

	zval property_is_no_auto_undo_default_value;
	ZVAL_UNDEF(&property_is_no_auto_undo_default_value);
	zend_string *property_is_no_auto_undo_name = zend_string_init("is_no_auto_undo", sizeof("is_no_auto_undo") - 1, true);
	zend_declare_typed_property(class_entry, property_is_no_auto_undo_name, &property_is_no_auto_undo_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_is_no_auto_undo_name, true);

	zval property_isolation_mode_default_value;
	ZVAL_UNDEF(&property_isolation_mode_default_value);
	zend_string *property_isolation_mode_name = zend_string_init("isolation_mode", sizeof("isolation_mode") - 1, true);
	zend_declare_typed_property(class_entry, property_isolation_mode_name, &property_isolation_mode_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_isolation_mode_name, true);

	zval property_lock_timeout_default_value;
	ZVAL_UNDEF(&property_lock_timeout_default_value);
	zend_string *property_lock_timeout_name = zend_string_init("lock_timeout", sizeof("lock_timeout") - 1, true);
	zend_declare_typed_property(class_entry, property_lock_timeout_name, &property_lock_timeout_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_lock_timeout_name, true);

	zval property_snapshot_at_number_default_value;
	ZVAL_UNDEF(&property_snapshot_at_number_default_value);
	zend_string *property_snapshot_at_number_name = zend_string_init("snapshot_at_number", sizeof("snapshot_at_number") - 1, true);
	zend_declare_typed_property(class_entry, property_snapshot_at_number_name, &property_snapshot_at_number_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY|ZEND_ACC_VIRTUAL, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_snapshot_at_number_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Server_Info(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Server_Info", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_server_version_default_value;
	ZVAL_UNDEF(&property_server_version_default_value);
	zend_string *property_server_version_name = zend_string_init("server_version", sizeof("server_version") - 1, true);
	zend_declare_typed_property(class_entry, property_server_version_name, &property_server_version_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_server_version_name, true);

	zval property_implementation_default_value;
	ZVAL_UNDEF(&property_implementation_default_value);
	zend_string *property_implementation_name = zend_string_init("implementation", sizeof("implementation") - 1, true);
	zend_declare_typed_property(class_entry, property_implementation_name, &property_implementation_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_implementation_name, true);

	zval property_get_env_default_value;
	ZVAL_UNDEF(&property_get_env_default_value);
	zend_string *property_get_env_name = zend_string_init("get_env", sizeof("get_env") - 1, true);
	zend_declare_typed_property(class_entry, property_get_env_name, &property_get_env_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_get_env_name, true);

	zval property_get_env_lock_default_value;
	ZVAL_UNDEF(&property_get_env_lock_default_value);
	zend_string *property_get_env_lock_name = zend_string_init("get_env_lock", sizeof("get_env_lock") - 1, true);
	zend_declare_typed_property(class_entry, property_get_env_lock_name, &property_get_env_lock_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_get_env_lock_name, true);

	zval property_get_env_msg_default_value;
	ZVAL_UNDEF(&property_get_env_msg_default_value);
	zend_string *property_get_env_msg_name = zend_string_init("get_env_msg", sizeof("get_env_msg") - 1, true);
	zend_declare_typed_property(class_entry, property_get_env_msg_name, &property_get_env_msg_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_get_env_msg_name, true);

	zval property_user_dbpath_default_value;
	ZVAL_UNDEF(&property_user_dbpath_default_value);
	zend_string *property_user_dbpath_name = zend_string_init("user_dbpath", sizeof("user_dbpath") - 1, true);
	zend_declare_typed_property(class_entry, property_user_dbpath_name, &property_user_dbpath_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_user_dbpath_name, true);

	zval property_db_info_default_value;
	ZVAL_UNDEF(&property_db_info_default_value);
	zend_string *property_db_info_name = zend_string_init("db_info", sizeof("db_info") - 1, true);
	zend_string *property_db_info_class_FireBird_Server_Db_Info = zend_string_init("FireBird\\Server_Db_Info", sizeof("FireBird\\Server_Db_Info")-1, 1);
	zend_declare_typed_property(class_entry, property_db_info_name, &property_db_info_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_CLASS(property_db_info_class_FireBird_Server_Db_Info, 0, MAY_BE_NULL));
	zend_string_release_ex(property_db_info_name, true);

	zval property_users_default_value;
	ZVAL_UNDEF(&property_users_default_value);
	zend_string *property_users_name = zend_string_init("users", sizeof("users") - 1, true);
	zend_declare_typed_property(class_entry, property_users_name, &property_users_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY));
	zend_string_release_ex(property_users_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Server_Db_Info(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Server_Db_Info", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_num_att_default_value;
	ZVAL_UNDEF(&property_num_att_default_value);
	zend_string *property_num_att_name = zend_string_init("num_att", sizeof("num_att") - 1, true);
	zend_declare_typed_property(class_entry, property_num_att_name, &property_num_att_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_num_att_name, true);

	zval property_num_db_default_value;
	ZVAL_UNDEF(&property_num_db_default_value);
	zend_string *property_num_db_name = zend_string_init("num_db", sizeof("num_db") - 1, true);
	zend_declare_typed_property(class_entry, property_num_db_name, &property_num_db_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_num_db_name, true);

	zval property_dbname_default_value;
	ZVAL_UNDEF(&property_dbname_default_value);
	zend_string *property_dbname_name = zend_string_init("dbname", sizeof("dbname") - 1, true);
	zend_declare_typed_property(class_entry, property_dbname_name, &property_dbname_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY));
	zend_string_release_ex(property_dbname_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Server_User_Info(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Server_User_Info", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_username_default_value;
	ZVAL_UNDEF(&property_username_default_value);
	zend_string *property_username_name = zend_string_init("username", sizeof("username") - 1, true);
	zend_declare_typed_property(class_entry, property_username_name, &property_username_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_username_name, true);

	zval property_firstname_default_value;
	ZVAL_UNDEF(&property_firstname_default_value);
	zend_string *property_firstname_name = zend_string_init("firstname", sizeof("firstname") - 1, true);
	zend_declare_typed_property(class_entry, property_firstname_name, &property_firstname_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_firstname_name, true);

	zval property_middlename_default_value;
	ZVAL_UNDEF(&property_middlename_default_value);
	zend_string *property_middlename_name = zend_string_init("middlename", sizeof("middlename") - 1, true);
	zend_declare_typed_property(class_entry, property_middlename_name, &property_middlename_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_middlename_name, true);

	zval property_lastname_default_value;
	ZVAL_UNDEF(&property_lastname_default_value);
	zend_string *property_lastname_name = zend_string_init("lastname", sizeof("lastname") - 1, true);
	zend_declare_typed_property(class_entry, property_lastname_name, &property_lastname_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_lastname_name, true);

	zval property_password_default_value;
	ZVAL_UNDEF(&property_password_default_value);
	zend_string *property_password_name = zend_string_init("password", sizeof("password") - 1, true);
	zend_declare_typed_property(class_entry, property_password_name, &property_password_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_password_name, true);

	zval property_admin_default_value;
	ZVAL_UNDEF(&property_admin_default_value);
	zend_string *property_admin_name = zend_string_init("admin", sizeof("admin") - 1, true);
	zend_declare_typed_property(class_entry, property_admin_name, &property_admin_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_admin_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Error(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Error", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_error_msg_default_value;
	ZVAL_UNDEF(&property_error_msg_default_value);
	zend_string *property_error_msg_name = zend_string_init("error_msg", sizeof("error_msg") - 1, true);
	zend_declare_typed_property(class_entry, property_error_msg_name, &property_error_msg_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_error_msg_name, true);

	zval property_error_code_default_value;
	ZVAL_UNDEF(&property_error_code_default_value);
	zend_string *property_error_code_name = zend_string_init("error_code", sizeof("error_code") - 1, true);
	zend_declare_typed_property(class_entry, property_error_code_name, &property_error_code_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_error_code_name, true);

	zval property_error_code_long_default_value;
	ZVAL_UNDEF(&property_error_code_long_default_value);
	zend_string *property_error_code_long_name = zend_string_init("error_code_long", sizeof("error_code_long") - 1, true);
	zend_declare_typed_property(class_entry, property_error_code_long_name, &property_error_code_long_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_error_code_long_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Db_Info(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Db_Info", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_reads_default_value;
	ZVAL_UNDEF(&property_reads_default_value);
	zend_string *property_reads_name = zend_string_init("reads", sizeof("reads") - 1, true);
	zend_declare_typed_property(class_entry, property_reads_name, &property_reads_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_reads_name, true);

	zval property_writes_default_value;
	ZVAL_UNDEF(&property_writes_default_value);
	zend_string *property_writes_name = zend_string_init("writes", sizeof("writes") - 1, true);
	zend_declare_typed_property(class_entry, property_writes_name, &property_writes_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_writes_name, true);

	zval property_fetches_default_value;
	ZVAL_UNDEF(&property_fetches_default_value);
	zend_string *property_fetches_name = zend_string_init("fetches", sizeof("fetches") - 1, true);
	zend_declare_typed_property(class_entry, property_fetches_name, &property_fetches_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_fetches_name, true);

	zval property_marks_default_value;
	ZVAL_UNDEF(&property_marks_default_value);
	zend_string *property_marks_name = zend_string_init("marks", sizeof("marks") - 1, true);
	zend_declare_typed_property(class_entry, property_marks_name, &property_marks_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_marks_name, true);

	zval property_page_size_default_value;
	ZVAL_UNDEF(&property_page_size_default_value);
	zend_string *property_page_size_name = zend_string_init("page_size", sizeof("page_size") - 1, true);
	zend_declare_typed_property(class_entry, property_page_size_name, &property_page_size_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_page_size_name, true);

	zval property_num_buffers_default_value;
	ZVAL_UNDEF(&property_num_buffers_default_value);
	zend_string *property_num_buffers_name = zend_string_init("num_buffers", sizeof("num_buffers") - 1, true);
	zend_declare_typed_property(class_entry, property_num_buffers_name, &property_num_buffers_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_num_buffers_name, true);

	zval property_current_memory_default_value;
	ZVAL_UNDEF(&property_current_memory_default_value);
	zend_string *property_current_memory_name = zend_string_init("current_memory", sizeof("current_memory") - 1, true);
	zend_declare_typed_property(class_entry, property_current_memory_name, &property_current_memory_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_current_memory_name, true);

	zval property_max_memory_default_value;
	ZVAL_UNDEF(&property_max_memory_default_value);
	zend_string *property_max_memory_name = zend_string_init("max_memory", sizeof("max_memory") - 1, true);
	zend_declare_typed_property(class_entry, property_max_memory_name, &property_max_memory_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_max_memory_name, true);

	zval property_allocation_default_value;
	ZVAL_UNDEF(&property_allocation_default_value);
	zend_string *property_allocation_name = zend_string_init("allocation", sizeof("allocation") - 1, true);
	zend_declare_typed_property(class_entry, property_allocation_name, &property_allocation_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_allocation_name, true);

	zval property_attachment_id_default_value;
	ZVAL_UNDEF(&property_attachment_id_default_value);
	zend_string *property_attachment_id_name = zend_string_init("attachment_id", sizeof("attachment_id") - 1, true);
	zend_declare_typed_property(class_entry, property_attachment_id_name, &property_attachment_id_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_attachment_id_name, true);

	zval property_read_seq_count_default_value;
	ZVAL_UNDEF(&property_read_seq_count_default_value);
	zend_string *property_read_seq_count_name = zend_string_init("read_seq_count", sizeof("read_seq_count") - 1, true);
	zend_declare_typed_property(class_entry, property_read_seq_count_name, &property_read_seq_count_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_read_seq_count_name, true);

	zval property_read_idx_count_default_value;
	ZVAL_UNDEF(&property_read_idx_count_default_value);
	zend_string *property_read_idx_count_name = zend_string_init("read_idx_count", sizeof("read_idx_count") - 1, true);
	zend_declare_typed_property(class_entry, property_read_idx_count_name, &property_read_idx_count_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_read_idx_count_name, true);

	zval property_insert_count_default_value;
	ZVAL_UNDEF(&property_insert_count_default_value);
	zend_string *property_insert_count_name = zend_string_init("insert_count", sizeof("insert_count") - 1, true);
	zend_declare_typed_property(class_entry, property_insert_count_name, &property_insert_count_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_insert_count_name, true);

	zval property_update_count_default_value;
	ZVAL_UNDEF(&property_update_count_default_value);
	zend_string *property_update_count_name = zend_string_init("update_count", sizeof("update_count") - 1, true);
	zend_declare_typed_property(class_entry, property_update_count_name, &property_update_count_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_update_count_name, true);

	zval property_delete_count_default_value;
	ZVAL_UNDEF(&property_delete_count_default_value);
	zend_string *property_delete_count_name = zend_string_init("delete_count", sizeof("delete_count") - 1, true);
	zend_declare_typed_property(class_entry, property_delete_count_name, &property_delete_count_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_delete_count_name, true);

	zval property_backout_count_default_value;
	ZVAL_UNDEF(&property_backout_count_default_value);
	zend_string *property_backout_count_name = zend_string_init("backout_count", sizeof("backout_count") - 1, true);
	zend_declare_typed_property(class_entry, property_backout_count_name, &property_backout_count_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_backout_count_name, true);

	zval property_purge_count_default_value;
	ZVAL_UNDEF(&property_purge_count_default_value);
	zend_string *property_purge_count_name = zend_string_init("purge_count", sizeof("purge_count") - 1, true);
	zend_declare_typed_property(class_entry, property_purge_count_name, &property_purge_count_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_purge_count_name, true);

	zval property_expunge_count_default_value;
	ZVAL_UNDEF(&property_expunge_count_default_value);
	zend_string *property_expunge_count_name = zend_string_init("expunge_count", sizeof("expunge_count") - 1, true);
	zend_declare_typed_property(class_entry, property_expunge_count_name, &property_expunge_count_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_expunge_count_name, true);

	zval property_isc_version_default_value;
	ZVAL_UNDEF(&property_isc_version_default_value);
	zend_string *property_isc_version_name = zend_string_init("isc_version", sizeof("isc_version") - 1, true);
	zend_declare_typed_property(class_entry, property_isc_version_name, &property_isc_version_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY));
	zend_string_release_ex(property_isc_version_name, true);

	zval property_firebird_version_default_value;
	ZVAL_UNDEF(&property_firebird_version_default_value);
	zend_string *property_firebird_version_name = zend_string_init("firebird_version", sizeof("firebird_version") - 1, true);
	zend_declare_typed_property(class_entry, property_firebird_version_name, &property_firebird_version_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY));
	zend_string_release_ex(property_firebird_version_name, true);

	zval property_limbo_default_value;
	ZVAL_UNDEF(&property_limbo_default_value);
	zend_string *property_limbo_name = zend_string_init("limbo", sizeof("limbo") - 1, true);
	zend_declare_typed_property(class_entry, property_limbo_name, &property_limbo_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY));
	zend_string_release_ex(property_limbo_name, true);

	zval property_sweep_interval_default_value;
	ZVAL_UNDEF(&property_sweep_interval_default_value);
	zend_string *property_sweep_interval_name = zend_string_init("sweep_interval", sizeof("sweep_interval") - 1, true);
	zend_declare_typed_property(class_entry, property_sweep_interval_name, &property_sweep_interval_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_sweep_interval_name, true);

	zval property_ods_version_default_value;
	ZVAL_UNDEF(&property_ods_version_default_value);
	zend_string *property_ods_version_name = zend_string_init("ods_version", sizeof("ods_version") - 1, true);
	zend_declare_typed_property(class_entry, property_ods_version_name, &property_ods_version_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_ods_version_name, true);

	zval property_ods_minor_version_default_value;
	ZVAL_UNDEF(&property_ods_minor_version_default_value);
	zend_string *property_ods_minor_version_name = zend_string_init("ods_minor_version", sizeof("ods_minor_version") - 1, true);
	zend_declare_typed_property(class_entry, property_ods_minor_version_name, &property_ods_minor_version_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_ods_minor_version_name, true);

	zval property_no_reserve_default_value;
	ZVAL_UNDEF(&property_no_reserve_default_value);
	zend_string *property_no_reserve_name = zend_string_init("no_reserve", sizeof("no_reserve") - 1, true);
	zend_declare_typed_property(class_entry, property_no_reserve_name, &property_no_reserve_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_no_reserve_name, true);

	zval property_forced_writes_default_value;
	ZVAL_UNDEF(&property_forced_writes_default_value);
	zend_string *property_forced_writes_name = zend_string_init("forced_writes", sizeof("forced_writes") - 1, true);
	zend_declare_typed_property(class_entry, property_forced_writes_name, &property_forced_writes_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_forced_writes_name, true);

	zval property_set_page_buffers_default_value;
	ZVAL_UNDEF(&property_set_page_buffers_default_value);
	zend_string *property_set_page_buffers_name = zend_string_init("set_page_buffers", sizeof("set_page_buffers") - 1, true);
	zend_declare_typed_property(class_entry, property_set_page_buffers_name, &property_set_page_buffers_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_set_page_buffers_name, true);

	zval property_db_sql_dialect_default_value;
	ZVAL_UNDEF(&property_db_sql_dialect_default_value);
	zend_string *property_db_sql_dialect_name = zend_string_init("db_sql_dialect", sizeof("db_sql_dialect") - 1, true);
	zend_declare_typed_property(class_entry, property_db_sql_dialect_name, &property_db_sql_dialect_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_db_sql_dialect_name, true);

	zval property_db_read_only_default_value;
	ZVAL_UNDEF(&property_db_read_only_default_value);
	zend_string *property_db_read_only_name = zend_string_init("db_read_only", sizeof("db_read_only") - 1, true);
	zend_declare_typed_property(class_entry, property_db_read_only_name, &property_db_read_only_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_db_read_only_name, true);

	zval property_db_size_in_pages_default_value;
	ZVAL_UNDEF(&property_db_size_in_pages_default_value);
	zend_string *property_db_size_in_pages_name = zend_string_init("db_size_in_pages", sizeof("db_size_in_pages") - 1, true);
	zend_declare_typed_property(class_entry, property_db_size_in_pages_name, &property_db_size_in_pages_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_db_size_in_pages_name, true);

	zval property_oldest_transaction_default_value;
	ZVAL_UNDEF(&property_oldest_transaction_default_value);
	zend_string *property_oldest_transaction_name = zend_string_init("oldest_transaction", sizeof("oldest_transaction") - 1, true);
	zend_declare_typed_property(class_entry, property_oldest_transaction_name, &property_oldest_transaction_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_oldest_transaction_name, true);

	zval property_oldest_active_default_value;
	ZVAL_UNDEF(&property_oldest_active_default_value);
	zend_string *property_oldest_active_name = zend_string_init("oldest_active", sizeof("oldest_active") - 1, true);
	zend_declare_typed_property(class_entry, property_oldest_active_name, &property_oldest_active_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_oldest_active_name, true);

	zval property_oldest_snapshot_default_value;
	ZVAL_UNDEF(&property_oldest_snapshot_default_value);
	zend_string *property_oldest_snapshot_name = zend_string_init("oldest_snapshot", sizeof("oldest_snapshot") - 1, true);
	zend_declare_typed_property(class_entry, property_oldest_snapshot_name, &property_oldest_snapshot_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_oldest_snapshot_name, true);

	zval property_next_transaction_default_value;
	ZVAL_UNDEF(&property_next_transaction_default_value);
	zend_string *property_next_transaction_name = zend_string_init("next_transaction", sizeof("next_transaction") - 1, true);
	zend_declare_typed_property(class_entry, property_next_transaction_name, &property_next_transaction_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_next_transaction_name, true);

	zval property_creation_date_default_value;
	ZVAL_UNDEF(&property_creation_date_default_value);
	zend_string *property_creation_date_name = zend_string_init("creation_date", sizeof("creation_date") - 1, true);
	zend_declare_typed_property(class_entry, property_creation_date_name, &property_creation_date_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_creation_date_name, true);

	zval property_db_file_size_default_value;
	ZVAL_UNDEF(&property_db_file_size_default_value);
	zend_string *property_db_file_size_name = zend_string_init("db_file_size", sizeof("db_file_size") - 1, true);
	zend_declare_typed_property(class_entry, property_db_file_size_name, &property_db_file_size_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_db_file_size_name, true);

	zval property_pages_used_default_value;
	ZVAL_UNDEF(&property_pages_used_default_value);
	zend_string *property_pages_used_name = zend_string_init("pages_used", sizeof("pages_used") - 1, true);
	zend_declare_typed_property(class_entry, property_pages_used_name, &property_pages_used_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_pages_used_name, true);

	zval property_pages_free_default_value;
	ZVAL_UNDEF(&property_pages_free_default_value);
	zend_string *property_pages_free_name = zend_string_init("pages_free", sizeof("pages_free") - 1, true);
	zend_declare_typed_property(class_entry, property_pages_free_name, &property_pages_free_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_pages_free_name, true);

	zval property_ses_idle_timeout_db_default_value;
	ZVAL_UNDEF(&property_ses_idle_timeout_db_default_value);
	zend_string *property_ses_idle_timeout_db_name = zend_string_init("ses_idle_timeout_db", sizeof("ses_idle_timeout_db") - 1, true);
	zend_declare_typed_property(class_entry, property_ses_idle_timeout_db_name, &property_ses_idle_timeout_db_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_ses_idle_timeout_db_name, true);

	zval property_ses_idle_timeout_att_default_value;
	ZVAL_UNDEF(&property_ses_idle_timeout_att_default_value);
	zend_string *property_ses_idle_timeout_att_name = zend_string_init("ses_idle_timeout_att", sizeof("ses_idle_timeout_att") - 1, true);
	zend_declare_typed_property(class_entry, property_ses_idle_timeout_att_name, &property_ses_idle_timeout_att_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_ses_idle_timeout_att_name, true);

	zval property_ses_idle_timeout_run_default_value;
	ZVAL_UNDEF(&property_ses_idle_timeout_run_default_value);
	zend_string *property_ses_idle_timeout_run_name = zend_string_init("ses_idle_timeout_run", sizeof("ses_idle_timeout_run") - 1, true);
	zend_declare_typed_property(class_entry, property_ses_idle_timeout_run_name, &property_ses_idle_timeout_run_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_ses_idle_timeout_run_name, true);

	zval property_conn_flags_default_value;
	ZVAL_UNDEF(&property_conn_flags_default_value);
	zend_string *property_conn_flags_name = zend_string_init("conn_flags", sizeof("conn_flags") - 1, true);
	zend_declare_typed_property(class_entry, property_conn_flags_name, &property_conn_flags_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_conn_flags_name, true);

	zval property_crypt_key_default_value;
	ZVAL_UNDEF(&property_crypt_key_default_value);
	zend_string *property_crypt_key_name = zend_string_init("crypt_key", sizeof("crypt_key") - 1, true);
	zend_declare_typed_property(class_entry, property_crypt_key_name, &property_crypt_key_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_crypt_key_name, true);

	zval property_crypt_state_default_value;
	ZVAL_UNDEF(&property_crypt_state_default_value);
	zend_string *property_crypt_state_name = zend_string_init("crypt_state", sizeof("crypt_state") - 1, true);
	zend_declare_typed_property(class_entry, property_crypt_state_name, &property_crypt_state_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_crypt_state_name, true);

	zval property_statement_timeout_db_default_value;
	ZVAL_UNDEF(&property_statement_timeout_db_default_value);
	zend_string *property_statement_timeout_db_name = zend_string_init("statement_timeout_db", sizeof("statement_timeout_db") - 1, true);
	zend_declare_typed_property(class_entry, property_statement_timeout_db_name, &property_statement_timeout_db_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_statement_timeout_db_name, true);

	zval property_statement_timeout_att_default_value;
	ZVAL_UNDEF(&property_statement_timeout_att_default_value);
	zend_string *property_statement_timeout_att_name = zend_string_init("statement_timeout_att", sizeof("statement_timeout_att") - 1, true);
	zend_declare_typed_property(class_entry, property_statement_timeout_att_name, &property_statement_timeout_att_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_statement_timeout_att_name, true);

	zval property_protocol_version_default_value;
	ZVAL_UNDEF(&property_protocol_version_default_value);
	zend_string *property_protocol_version_name = zend_string_init("protocol_version", sizeof("protocol_version") - 1, true);
	zend_declare_typed_property(class_entry, property_protocol_version_name, &property_protocol_version_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_protocol_version_name, true);

	zval property_crypt_plugin_default_value;
	ZVAL_UNDEF(&property_crypt_plugin_default_value);
	zend_string *property_crypt_plugin_name = zend_string_init("crypt_plugin", sizeof("crypt_plugin") - 1, true);
	zend_declare_typed_property(class_entry, property_crypt_plugin_name, &property_crypt_plugin_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_crypt_plugin_name, true);

	zval property_wire_crypt_default_value;
	ZVAL_UNDEF(&property_wire_crypt_default_value);
	zend_string *property_wire_crypt_name = zend_string_init("wire_crypt", sizeof("wire_crypt") - 1, true);
	zend_declare_typed_property(class_entry, property_wire_crypt_name, &property_wire_crypt_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_wire_crypt_name, true);

	zval property_next_attachment_default_value;
	ZVAL_UNDEF(&property_next_attachment_default_value);
	zend_string *property_next_attachment_name = zend_string_init("next_attachment", sizeof("next_attachment") - 1, true);
	zend_declare_typed_property(class_entry, property_next_attachment_name, &property_next_attachment_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_next_attachment_name, true);

	zval property_next_statement_default_value;
	ZVAL_UNDEF(&property_next_statement_default_value);
	zend_string *property_next_statement_name = zend_string_init("next_statement", sizeof("next_statement") - 1, true);
	zend_declare_typed_property(class_entry, property_next_statement_name, &property_next_statement_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_next_statement_name, true);

	zval property_db_guid_default_value;
	ZVAL_UNDEF(&property_db_guid_default_value);
	zend_string *property_db_guid_name = zend_string_init("db_guid", sizeof("db_guid") - 1, true);
	zend_declare_typed_property(class_entry, property_db_guid_name, &property_db_guid_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_db_guid_name, true);

	zval property_db_file_id_default_value;
	ZVAL_UNDEF(&property_db_file_id_default_value);
	zend_string *property_db_file_id_name = zend_string_init("db_file_id", sizeof("db_file_id") - 1, true);
	zend_declare_typed_property(class_entry, property_db_file_id_name, &property_db_file_id_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_db_file_id_name, true);

	zval property_replica_mode_default_value;
	ZVAL_UNDEF(&property_replica_mode_default_value);
	zend_string *property_replica_mode_name = zend_string_init("replica_mode", sizeof("replica_mode") - 1, true);
	zend_declare_typed_property(class_entry, property_replica_mode_name, &property_replica_mode_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_replica_mode_name, true);

	zval property_username_default_value;
	ZVAL_UNDEF(&property_username_default_value);
	zend_string *property_username_name = zend_string_init("username", sizeof("username") - 1, true);
	zend_declare_typed_property(class_entry, property_username_name, &property_username_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_username_name, true);

	zval property_sqlrole_default_value;
	ZVAL_UNDEF(&property_sqlrole_default_value);
	zend_string *property_sqlrole_name = zend_string_init("sqlrole", sizeof("sqlrole") - 1, true);
	zend_declare_typed_property(class_entry, property_sqlrole_name, &property_sqlrole_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_sqlrole_name, true);

	zval property_parallel_workers_default_value;
	ZVAL_UNDEF(&property_parallel_workers_default_value);
	zend_string *property_parallel_workers_name = zend_string_init("parallel_workers", sizeof("parallel_workers") - 1, true);
	zend_declare_typed_property(class_entry, property_parallel_workers_name, &property_parallel_workers_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_parallel_workers_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Var_Info(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Var_Info", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_name_default_value;
	ZVAL_UNDEF(&property_name_default_value);
	zend_declare_typed_property(class_entry, ZSTR_KNOWN(ZEND_STR_NAME), &property_name_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));

	zval property_alias_default_value;
	ZVAL_UNDEF(&property_alias_default_value);
	zend_string *property_alias_name = zend_string_init("alias", sizeof("alias") - 1, true);
	zend_declare_typed_property(class_entry, property_alias_name, &property_alias_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_alias_name, true);

	zval property_relation_default_value;
	ZVAL_UNDEF(&property_relation_default_value);
	zend_string *property_relation_name = zend_string_init("relation", sizeof("relation") - 1, true);
	zend_declare_typed_property(class_entry, property_relation_name, &property_relation_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_relation_name, true);

	zval property_byte_length_default_value;
	ZVAL_UNDEF(&property_byte_length_default_value);
	zend_string *property_byte_length_name = zend_string_init("byte_length", sizeof("byte_length") - 1, true);
	zend_declare_typed_property(class_entry, property_byte_length_name, &property_byte_length_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_byte_length_name, true);

	zval property_type_default_value;
	ZVAL_UNDEF(&property_type_default_value);
	zend_declare_typed_property(class_entry, ZSTR_KNOWN(ZEND_STR_TYPE), &property_type_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));

	zval property_sub_type_default_value;
	ZVAL_UNDEF(&property_sub_type_default_value);
	zend_string *property_sub_type_name = zend_string_init("sub_type", sizeof("sub_type") - 1, true);
	zend_declare_typed_property(class_entry, property_sub_type_name, &property_sub_type_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_sub_type_name, true);

	zval property_scale_default_value;
	ZVAL_UNDEF(&property_scale_default_value);
	zend_string *property_scale_name = zend_string_init("scale", sizeof("scale") - 1, true);
	zend_declare_typed_property(class_entry, property_scale_name, &property_scale_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_scale_name, true);

	zval property_nullable_default_value;
	ZVAL_UNDEF(&property_nullable_default_value);
	zend_string *property_nullable_name = zend_string_init("nullable", sizeof("nullable") - 1, true);
	zend_declare_typed_property(class_entry, property_nullable_name, &property_nullable_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_nullable_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Blob_Id(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Blob_Id", class_FireBird_Blob_Id_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Connect_Args(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Connect_Args", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_database_default_value;
	ZVAL_UNDEF(&property_database_default_value);
	zend_string *property_database_name = zend_string_init("database", sizeof("database") - 1, true);
	zend_declare_typed_property(class_entry, property_database_name, &property_database_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_database_name, true);

	zval property_user_name_default_value;
	ZVAL_UNDEF(&property_user_name_default_value);
	zend_string *property_user_name_name = zend_string_init("user_name", sizeof("user_name") - 1, true);
	zend_declare_typed_property(class_entry, property_user_name_name, &property_user_name_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_user_name_name, true);

	zval property_password_default_value;
	ZVAL_UNDEF(&property_password_default_value);
	zend_string *property_password_name = zend_string_init("password", sizeof("password") - 1, true);
	zend_declare_typed_property(class_entry, property_password_name, &property_password_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_password_name, true);

	zval property_role_name_default_value;
	ZVAL_UNDEF(&property_role_name_default_value);
	zend_string *property_role_name_name = zend_string_init("role_name", sizeof("role_name") - 1, true);
	zend_declare_typed_property(class_entry, property_role_name_name, &property_role_name_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_role_name_name, true);

	zval property_charset_default_value;
	ZVAL_UNDEF(&property_charset_default_value);
	zend_string *property_charset_name = zend_string_init("charset", sizeof("charset") - 1, true);
	zend_declare_typed_property(class_entry, property_charset_name, &property_charset_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_charset_name, true);

	zval property_num_buffers_default_value;
	ZVAL_UNDEF(&property_num_buffers_default_value);
	zend_string *property_num_buffers_name = zend_string_init("num_buffers", sizeof("num_buffers") - 1, true);
	zend_declare_typed_property(class_entry, property_num_buffers_name, &property_num_buffers_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_num_buffers_name, true);

	zval property_timeout_default_value;
	ZVAL_UNDEF(&property_timeout_default_value);
	zend_string *property_timeout_name = zend_string_init("timeout", sizeof("timeout") - 1, true);
	zend_declare_typed_property(class_entry, property_timeout_name, &property_timeout_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_timeout_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Create_Args(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Create_Args", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_database_default_value;
	ZVAL_UNDEF(&property_database_default_value);
	zend_string *property_database_name = zend_string_init("database", sizeof("database") - 1, true);
	zend_declare_typed_property(class_entry, property_database_name, &property_database_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_database_name, true);

	zval property_user_name_default_value;
	ZVAL_UNDEF(&property_user_name_default_value);
	zend_string *property_user_name_name = zend_string_init("user_name", sizeof("user_name") - 1, true);
	zend_declare_typed_property(class_entry, property_user_name_name, &property_user_name_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_user_name_name, true);

	zval property_password_default_value;
	ZVAL_UNDEF(&property_password_default_value);
	zend_string *property_password_name = zend_string_init("password", sizeof("password") - 1, true);
	zend_declare_typed_property(class_entry, property_password_name, &property_password_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_password_name, true);

	zval property_set_db_charset_default_value;
	ZVAL_UNDEF(&property_set_db_charset_default_value);
	zend_string *property_set_db_charset_name = zend_string_init("set_db_charset", sizeof("set_db_charset") - 1, true);
	zend_declare_typed_property(class_entry, property_set_db_charset_name, &property_set_db_charset_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_set_db_charset_name, true);

	zval property_sweep_interval_default_value;
	ZVAL_UNDEF(&property_sweep_interval_default_value);
	zend_string *property_sweep_interval_name = zend_string_init("sweep_interval", sizeof("sweep_interval") - 1, true);
	zend_declare_typed_property(class_entry, property_sweep_interval_name, &property_sweep_interval_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_sweep_interval_name, true);

	zval property_set_page_buffers_default_value;
	ZVAL_UNDEF(&property_set_page_buffers_default_value);
	zend_string *property_set_page_buffers_name = zend_string_init("set_page_buffers", sizeof("set_page_buffers") - 1, true);
	zend_declare_typed_property(class_entry, property_set_page_buffers_name, &property_set_page_buffers_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_set_page_buffers_name, true);

	zval property_page_size_default_value;
	ZVAL_UNDEF(&property_page_size_default_value);
	zend_string *property_page_size_name = zend_string_init("page_size", sizeof("page_size") - 1, true);
	zend_declare_typed_property(class_entry, property_page_size_name, &property_page_size_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_page_size_name, true);

	zval property_force_write_default_value;
	ZVAL_UNDEF(&property_force_write_default_value);
	zend_string *property_force_write_name = zend_string_init("force_write", sizeof("force_write") - 1, true);
	zend_declare_typed_property(class_entry, property_force_write_name, &property_force_write_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_force_write_name, true);

	zval property_overwrite_default_value;
	ZVAL_UNDEF(&property_overwrite_default_value);
	zend_string *property_overwrite_name = zend_string_init("overwrite", sizeof("overwrite") - 1, true);
	zend_declare_typed_property(class_entry, property_overwrite_name, &property_overwrite_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_BOOL));
	zend_string_release_ex(property_overwrite_name, true);

	zval property_timeout_default_value;
	ZVAL_UNDEF(&property_timeout_default_value);
	zend_string *property_timeout_name = zend_string_init("timeout", sizeof("timeout") - 1, true);
	zend_declare_typed_property(class_entry, property_timeout_name, &property_timeout_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release_ex(property_timeout_name, true);

	return class_entry;
}

static zend_class_entry *register_class_FireBird_Service_Connect_Args(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FireBird", "Service_Connect_Args", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, 0);

	zval property_service_name_default_value;
	ZVAL_UNDEF(&property_service_name_default_value);
	zend_string *property_service_name_name = zend_string_init("service_name", sizeof("service_name") - 1, true);
	zend_declare_typed_property(class_entry, property_service_name_name, &property_service_name_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_service_name_name, true);

	zval property_user_name_default_value;
	ZVAL_UNDEF(&property_user_name_default_value);
	zend_string *property_user_name_name = zend_string_init("user_name", sizeof("user_name") - 1, true);
	zend_declare_typed_property(class_entry, property_user_name_name, &property_user_name_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_user_name_name, true);

	zval property_password_default_value;
	ZVAL_UNDEF(&property_password_default_value);
	zend_string *property_password_name = zend_string_init("password", sizeof("password") - 1, true);
	zend_declare_typed_property(class_entry, property_password_name, &property_password_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_password_name, true);

	return class_entry;
}
