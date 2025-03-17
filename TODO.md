# TODO
## Blobs
- seek (for streaming blobs)
- add type (segmented / streamed) information to blob info
- blob __toString() return old style blob id

## Explore IBatch
- When inserting/updating large amounts of data.
- When reducing network round trips is critical.
- When handling bulk inserts with RETURNING values.
- When performing batch deletes/updates efficiently.
IStatement::createBatch, IAttachment::createBatch
