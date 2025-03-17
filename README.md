# Current progress

|     | Function                 | Notes                                 |
|---  |---                       | ---                                   |
|❌    |ibase_add_user            |                                       |
|✅    | ibase_affected_rows      |                                       |
|❌    | ibase_backup             |                                       |
|✅    | ibase_blob_add           |                                       |
|✅    | ibase_blob_cancel        |                                       |
|✅    | ibase_blob_close         |                                       |
|✅    | ibase_blob_create        |                                       |
|🚫   | ibase_blob_echo          | can be easily done in PHP space       |
|✅    | ibase_blob_get           |                                       |
|🚫   | ibase_blob_import        | can be easily done in PHP space       |
|✅    | ibase_blob_info          |                                       |
|✅    | ibase_blob_open          |                                       |
|✅    | ibase_close              |                                       |
|✅    | ibase_commit             |                                       |
|✅    | ibase_commit_ret         |                                       |
|✅    | ibase_connect            |                                       |
|❌    | ibase_db_info            |                                       |
|❌    | ibase_delete_user        |                                       |
|✅    | ibase_drop_db            |                                       |
|✅    | ibase_errcode            |                                       |
|✅    | ibase_errmsg             |                                       |
|✅    | ibase_execute            |                                       |
|✅    | ibase_fetch_assoc        |                                       |
|✅    | ibase_fetch_object       |                                       |
|✅    | ibase_fetch_row          |                                       |
|✅    | ibase_field_info         |                                       |
|❌    | ibase_free_event_handler |                                       |
|✅    | ibase_free_query         |                                       |
|✅    | ibase_free_result        |                                       |
|🚫   | ibase_gen_id             | can be easily done in PHP space       |
|❌    | ibase_maintain_db        |                                       |
|❌    | ibase_modify_user        |                                       |
|✅    | ibase_name_result        |                                       |
|✅    | ibase_num_fields         |                                       |
|✅    | ibase_num_params         |                                       |
|✅    | ibase_param_info         |                                       |
|❌    | ibase_pconnect           |                                       |
|✅    | ibase_prepare            |                                       |
|✅    | ibase_query              |                                       |
|❌    | ibase_restore            |                                       |
|✅    | ibase_rollback           |                                       |
|✅    | ibase_rollback_ret       |                                       |
|❌    | ibase_server_info        |                                       |
|❌    | ibase_service_attach     |                                       |
|❌    | ibase_service_detach     |                                       |
|❌    | ibase_set_event_handler  |                                       |
|⏳    | ibase_trans              | Implement multi database transactions |
|❌    | ibase_wait_event         |                                       |

# TODO

## General

- Collect opened statements/transactions/blobs/etc and report leaks

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
