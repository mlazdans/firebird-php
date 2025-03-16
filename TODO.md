# Explore IBatch

- When inserting/updating large amounts of data.
- When reducing network round trips is critical.
- When handling bulk inserts with RETURNING values.
- When performing batch deletes/updates efficiently.

IStatement::createBatch, IAttachment::createBatch


# Explore streamed blobs

BLOB_open(), BLOB_put(), ...

ibase_add_user — Add a user to a security database
ibase_affected_rows — Return the number of rows that were affected by the previous query
ibase_backup — Initiates a backup task in the service manager and returns immediately
✅ ibase_blob_add — Add data into a newly created blob
✅ ibase_blob_cancel — Cancel creating blob
✅ ibase_blob_close — Close blob
✅ ibase_blob_create — Create a new blob for adding data
❎ ibase_blob_echo — Output blob contents to browser
✅ ibase_blob_get — Get len bytes data from open blob
❎ ibase_blob_import — Create blob, copy file in it, and close it
✅ ibase_blob_info — Return blob length and other useful info
✅ ibase_blob_open — Open blob for retrieving data parts
✅ ibase_close — Close a connection to an InterBase database
✅ ibase_commit — Commit a transaction
✅ ibase_commit_ret — Commit a transaction without closing it
✅ ibase_connect — Open a connection to a database
ibase_db_info — Request statistics about a database
ibase_delete_user — Delete a user from a security database
✅ ibase_drop_db — Drops a database
✅ ibase_errcode — Return an error code
✅ ibase_errmsg — Return error messages
✅ ibase_execute — Execute a previously prepared query
✅ ibase_fetch_assoc — Fetch a result row from a query as an associative array
✅ ibase_fetch_object — Get an object from a InterBase database
✅ ibase_fetch_row — Fetch a row from an InterBase database
ibase_field_info — Get information about a field
ibase_free_event_handler — Cancels a registered event handler
✅ ibase_free_query — Free memory allocated by a prepared query
✅ ibase_free_result — Free a result set
ibase_gen_id — Increments the named generator and returns its new value
ibase_maintain_db — Execute a maintenance command on the database server
ibase_modify_user — Modify a user to a security database
ibase_name_result — Assigns a name to a result set
✅ ibase_num_fields — Get the number of fields in a result set
✅ ibase_num_params — Return the number of parameters in a prepared query
ibase_param_info — Return information about a parameter in a prepared query
ibase_pconnect — Open a persistent connection to an InterBase database
✅ ibase_prepare — Prepare a query for later binding of parameter placeholders and execution
✅ ibase_query — Execute a query on an InterBase database
ibase_restore — Initiates a restore task in the service manager and returns immediately
✅ ibase_rollback — Roll back a transaction
✅ ibase_rollback_ret — Roll back a transaction without closing it
ibase_server_info — Request information about a database server
ibase_service_attach — Connect to the service manager
ibase_service_detach — Disconnect from the service manager
ibase_set_event_handler — Register a callback function to be called when events are posted
✅ ibase_trans — Begin a transaction
ibase_wait_event — Wait for an event to be posted by the database
