<?php declare(strict_types = 1);

namespace FireBird;

const FETCH_BLOBS    = 1;
const FETCH_ARRAYS   = 2;
const FETCH_UNIXTIME = 4;

// TODO: private __construct()
// Store single Firebird error
class Error
{
    public string $error_msg { get; }
    public int $error_code { get; }
    public int $error_code_long { get; }
}

class Blob_Info
{
    public int $num_segments { get; }
    public int $max_segment { get; }
    public int $total_length { get; }
    public int $type { get; }
}

class Blob_Id
{
}

interface IError
{
    public string $error_msg { get; }    // Combined multi-line error message from $errors
    public int $error_code { get; }      // Last error_code from $errors
    public int $error_code_long { get; } // Last error_code_long from $errors

    public string $error_file { get; }   // PHP file
    public int $error_lineno { get; }    // PHP file's line number

    /** @var Error[]  */
    public array $errors { get; }
}

// There is no internal trait. Used for stub only
trait FB_Error
{
    protected(set) string $error_msg;
    protected(set) int $error_code;
    protected(set) int $error_code_long;

    protected(set) string $error_file;
    protected(set) int $error_lineno;

    protected(set) array $errors;        // array of all Firebird errors
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

class Database implements IError
{
    use FB_Error;

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
    use FB_Error;

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
    use FB_Error;

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

    /** @return Blob|false */
    function open_blob(Blob_Id $id) {}

    /** @return Blob|false */
    function create_blob() {}
}

class Statement implements IError
{
    use FB_Error;

    protected(set) Transaction $transaction;
    protected(set) int $num_parameters_in;
    protected(set) int $num_parameters_out;

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

    /** @return bool */
    function free() {}
}

class Blob implements IError
{
    use FB_Error;

    protected(set) Transaction $transaction;

    private function __construct() {}

    /** @return bool */
    function close() {}

    /** @return bool */
    function cancel() {}

    /** @return Blob_Info|false */
    function info() {}

    /** @return string|false */
    function get(int $max_len = 0) {}

    /** @return bool */
    function put(string $data) {}
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

// class Event {
// }

// class Service {
//     function add_user() {}
//     function db_info() {}
//     function backup() {}
//     function delete_user() {}
//     function server_info() {}
// }
