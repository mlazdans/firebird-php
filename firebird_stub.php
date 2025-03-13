<?php declare(strict_types = 1);

namespace FireBird;

const FETCH_BLOBS    = 1;
const FETCH_ARRAYS   = 2;
const FETCH_UNIXTIME = 4;

interface IError
{
    public string $error_msg { get; }
    public int $error_code { get; }
    public int $error_code_long { get; }
}

trait Error
{
    protected(set) string $error_msg;
    protected(set) int $error_code;
    protected(set) int $error_code_long;
}

class Connect_Args
{
    var string $database;
    var string $user_name;
    var string $password;
    var string $role_name;
    var string $charset;
    var int $num_buffers;
}

class Create_Args
{
    var string $database;
    var string $user_name;
    var string $password;
    var string $set_db_charset;
    var int $sweep_interval;
    var int $set_page_buffers;
    var int $page_size;
    var bool $force_write;
    var bool $overwrite;
}

// TODO: static class with utilities
class Database implements IError
{
    use Error;

    protected(set) Connect_Args|Create_Args $args;

    /** @return Connection|false */
    function connect(Connect_Args $args) {}

    /** @return Connection|false */
    function create(Create_Args $args) {}

    /** @return bool */
    function drop() {}
}

class Connection implements IError
{
    use Error;

    protected(set) Database $database;

    private function __construct() {}

    /**
     * @param ?int $lock_timeout - sets lock timeout in seconds when WAIT | LOCK_TIMEOUT $trans_args are set. Valid range 1-32767
     * @return Transaction|false
     * */
    function new_transaction(int $trans_args = 0, int $lock_timeout = 0) {}
    // TODO: new_transaction w/o starting or start_transaction?

    /** @return bool */
    function disconnect() {}
}

// TODO: auto commit/rollback flag?
class Transaction implements IError
{
    use Error;

    protected(set) Connection $connection;

    private function __construct() {}

    /** @return bool */
    function start() {}

    /** @return bool */
    function commit() {}

    /** @return bool */
    function commit_ret() {}

    /** @return bool */
    function rollback() {}

    /** @return bool */
    function rollback_ret() {}

    /** @return Statement|false */
    function query(string $sql, ...$bind_args) {}

    /** @return Statement|false */
    function prepare(string $sql) {}
}

class Statement implements IError
{
    use Error;

    private function __construct() {}

    /** @return object|false|null */
    function fetch_object(int $flags = 0) {}

    /** @return array|false|null */
    function fetch_array(int $flags = 0) {}

    /** @return array|false|null */
    function fetch_row(int $flags = 0) {}

    /** @return bool */
    function execute(...$bind_args) {}

    /** @return bool */
    function close() {}
}

namespace FireBird\Transaction;
const WRITE          = 1;
const READ           = 2;
const COMMITTED      = 8;
const CONSISTENCY    = 16;
const CONCURRENCY    = 4;
const REC_VERSION    = 64;
const REC_NO_VERSION = 32;
const NOWAIT         = 256;
const WAIT           = 128;
const LOCK_TIMEOUT   = 512;

// class Blob {
//     function add() {}
//     function cancel() {}
//     function close() {}
//     function create() {}
//     function echo() {}
//     function get() {}
//     function import() {}
//     function info() {}
//     function open() {}
// }

// class Event {
// }

// class Service {
//     function add_user() {}
//     function db_info() {}
//     function backup() {}
//     function delete_user() {}
//     function server_info() {}
// }
