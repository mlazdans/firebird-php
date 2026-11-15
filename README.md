# Firebird modern PHP extension

:warning: Development is still in progress, and the API is not yet stable.

This project aims to repackage the old [ibase](https://www.php.net/ibase)
extension into a more modern PHP framework. Internal resource types have been
removed and replaced with a object-oriented interface. Most of the ibase
extension features have now been ported; however, development is still in
progress, and the API is not yet stable.

Current development environment: PHP 8.4.4 NTS, Firebird 5.0.3, Debian 12 (64-bit).

Currently supported PHP versions: 8.4, 8.5, 8.6

# Example
```php
<?php declare(strict_types = 1);

$args = new \FireBird\Connect_Args;
$args->database = "localhost:/opt/db/test.fdb";
$args->user_name = "sysdba";
$args->password = "masterkey";

$db = \FireBird\Database::connect($args);
$t = $db->start_transaction();
$q = $t->query("SELECT * FROM TEST_TABLE");
while ($r = $q->fetch_object(\FireBird\FETCH_BLOB_TEXT)) {
    print_r($r);
}
```

Some example code can be fetched from [tests](tests/)

PHP stub file: [firebird.stub.php](firebird.stub.php)

# Build

## Linux
```
phpize
./configure --with-firebird
make
make install
```

Optionally specify Firebird 5.0 path: --with-firebird=/opt/firebird

## Windows

1. Install PHP SDK: https://github.com/microsoft/php-sdk-binary-tools
2. Install git, add to PATH
3. Adjust php-firebird-config.bat. PFB_SOURCE_DIR should point one level up, e.g. if you clone firebird-php into D:\firebird-php\firebird-php\, then PFB_SOURCE_DIR=D:\firebird-php\
4. cd C:\php-sdk\ and call D:\firebird-php\firebird-php\build_scripts\php-firebird-build-all.bat

# TODO

## Current progress

|     | Function                  | Notes |
| --- | ------------------------- |  ---  |
|❎    | ibase_add_user           | Manage via SQL |
|✅    | ibase_affected_rows      |      |
|❌    | ibase_backup             | C++ refactoring needed |
|✅    | ibase_blob_add           |      |
|✅    | ibase_blob_cancel        |      |
|✅    | ibase_blob_close         |      |
|✅    | ibase_blob_create        |      |
|❎    | ibase_blob_echo          | Can be easily done from PHP |
|✅    | ibase_blob_get           |      |
|❎    | ibase_blob_import        | Can be easily done from PHP |
|✅    | ibase_blob_info          |      |
|✅    | ibase_blob_open          |      |
|✅    | ibase_close              |      |
|✅    | ibase_commit             |      |
|✅    | ibase_commit_ret         |      |
|✅    | ibase_connect            |      |
|❎    | ibase_db_info            | Not worth it. It appears this returns unstructured data anyways. |
|❎    | ibase_delete_user        | Manage via SQL |
|✅    | ibase_drop_db            |      |
|✅    | ibase_errcode            | Via exceptions |
|✅    | ibase_errmsg             | Via exceptions |
|✅    | ibase_execute            |      |
|✅    | ibase_fetch_assoc        |      |
|✅    | ibase_fetch_object       |      |
|✅    | ibase_fetch_row          |      |
|✅    | ibase_field_info         |      |
|❌    | ibase_free_event_handler | C++ refactoring needed |
|✅    | ibase_free_query         |      |
|✅    | ibase_free_result        |      |
|❎    | ibase_gen_id             | Can be easily done from PHP |
|❌    | ibase_maintain_db        | C++ refactoring needed |
|❎    | ibase_modify_user        | Manage via SQL |
|❌    | ibase_name_result        | C++ refactoring needed |
|✅    | ibase_num_fields         |      |
|✅    | ibase_num_params         |      |
|✅    | ibase_param_info         |      |
|❓    | ibase_pconnect           | Not sure if this is a good idea. Most likely this will be used inproperly anyways, leaving around long running transactions. [ALTER SESSION RESET](https://firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-management-session-reset-alter)     |
|✅    | ibase_prepare            |      |
|✅    | ibase_query              |      |
|❌    | ibase_restore            | C++ refactoring needed |
|✅    | ibase_rollback           |      |
|✅    | ibase_rollback_ret       |      |
|✅    | ibase_server_info        |  |
|✅    | ibase_service_attach     |  |
|✅    | ibase_service_detach     |  |
|❌    | ibase_set_event_handler  |      |
|✅    | ibase_trans              |      |
|❌    | ibase_wait_event         |      |

## General

- Collect opened and not closed statements/transactions/blobs/etc and report

## Statement

- isc_info_sql_get_plan, isc_info_sql_explain_plan and other information

## Explore IBatch posibilities

IStatement::createBatch, IAttachment::createBatch

- When inserting/updating large amounts of data.
- When reducing network round trips is critical.
- When handling bulk inserts with RETURNING values.
- When performing batch deletes/updates efficiently.

# Arrays

Support for arrays are not planned. See more:
https://firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-datatypes-array
