# TODO
## Blob
- seek (for streaming blobs)
- add type (segmented / streamed) information to blob info
- blob __toString() return old style blob id

## Statement
- isc_info_sql_get_plan, isc_info_sql_explain_plan and other information

## Explore IBatch posibilities
- When inserting/updating large amounts of data.
- When reducing network round trips is critical.
- When handling bulk inserts with RETURNING values.
- When performing batch deletes/updates efficiently.
IStatement::createBatch, IAttachment::createBatch
