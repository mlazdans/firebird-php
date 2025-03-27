# Current progress

|     | Function                  | Notes                                 |
|---  |---                        | ---                                   |
|✅    | ibase_add_user           |                                       |
|✅    | ibase_affected_rows      |                                       |
|✅    | ibase_backup             | Simple backup w/o extra options       |
|✅    | ibase_blob_add           |                                       |
|✅    | ibase_blob_cancel        |                                       |
|✅    | ibase_blob_close         |                                       |
|✅    | ibase_blob_create        |                                       |
|🚫   | ibase_blob_echo           | Can be easily done from PHP           |
|✅    | ibase_blob_get           |                                       |
|🚫   | ibase_blob_import         | Can be easily done from PHP           |
|✅    | ibase_blob_info          |                                       |
|✅    | ibase_blob_open          |                                       |
|✅    | ibase_close              |                                       |
|✅    | ibase_commit             |                                       |
|✅    | ibase_commit_ret         |                                       |
|✅    | ibase_connect            |                                       |
|🚫    | ibase_db_info            | Not worth it. It appears this returns unstructured data anyways. |
|✅    | ibase_delete_user        |                                       |
|✅    | ibase_drop_db            |                                       |
|✅    | ibase_errcode            |                                       |
|✅    | ibase_errmsg             |                                       |
|✅    | ibase_execute            |                                       |
|✅    | ibase_fetch_assoc        |                                       |
|✅    | ibase_fetch_object       |                                       |
|✅    | ibase_fetch_row          |                                       |
|✅    | ibase_field_info         |                                       |
|✅    | ibase_free_event_handler |                                       |
|✅    | ibase_free_query         |                                       |
|✅    | ibase_free_result        |                                       |
|🚫   | ibase_gen_id             | Can be easily done from PHP            |
|✅    | ibase_maintain_db        |                                       |
|✅    | ibase_modify_user        |                                       |
|✅    | ibase_name_result        |                                       |
|✅    | ibase_num_fields         |                                       |
|✅    | ibase_num_params         |                                       |
|✅    | ibase_param_info         |                                       |
|❓    | ibase_pconnect           | Not sure if this is a good idea. Most likely this will be used inproperly anyways, leaving around long running transactions. [ALTER SESSION RESET](https://firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-management-session-reset-alter)     |
|✅    | ibase_prepare            |                                       |
|✅    | ibase_query              |                                       |
|✅    | ibase_restore            | Simple restore w/o extra options      |
|✅    | ibase_rollback           |                                       |
|✅    | ibase_rollback_ret       |                                       |
|✅    | ibase_server_info        |                                       |
|✅    | ibase_service_attach     |                                       |
|✅    | ibase_service_detach     |                                       |
|⏳    | ibase_set_event_handler  | Initial naive implementation          |
|⏳    | ibase_trans              | Yet to implement multi database transactions |
|⏳    | ibase_wait_event         | Initial naive implementation          |

# TODO

## General

- Collect opened statements/transactions/blobs/etc and report leaks
- isc_dsql_execute_immediate w/o prepare
- transaction over more databases: if (isc_start_transaction(status, &trans1, 2, &db1, 0, NULL, &db2, 0, NULL))
- add private __construct() to classes that is not constructable from PHP. Or accept appropriate object to construc, for exampl, new Transaction($connection)

## Blob

- seek (for streaming blobs)
- add type (segmented / streamed) information to blob info
- blob __toString() return old style blob id

## Statement

- isc_info_sql_get_plan, isc_info_sql_explain_plan and other information

## Explore IBatch posibilities

IStatement::createBatch, IAttachment::createBatch

- When inserting/updating large amounts of data.
- When reducing network round trips is critical.
- When handling bulk inserts with RETURNING values.
- When performing batch deletes/updates efficiently.

## Arrays

Is it worth even supporting array type? Firebird docs thinks not:
https://firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-datatypes-array

> Firebird does not offer much in the way of language or tools for working with the contents of arrays, and there are no plans to improve this. This limits the usefulness and accessibility of array types. Therefore, the general advice is: do not use arrays.

Arrays introduce unnecessary complexity. You can't even INSERT / UPDATE array fields using PSQL, just with isc API. If arrays are needed these can be implemented using relation model.
