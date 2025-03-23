<?php declare(strict_types = 1);

namespace FireBird;

const FETCH_BLOBS    = 1;
const FETCH_ARRAYS   = 2;
const FETCH_UNIXTIME = 4;

class Server_Info
{
    public string $server_version;
    public string $implementation;
    public string $get_env;
    public string $get_env_lock;
    public string $get_env_msg;
    public string $user_dbpath;
    public ?Server_Db_Info $db_info;

    /** @var Server_User_Info[] */
    public array $users;
}

class Server_Db_Info
{
    public int $num_att;
    public int $num_db;

    /** @var string[] */
    public array $dbname;
}

class Server_User_Info
{
    public string $username;
    public string $firstname;
    public string $middlename;
    public string $lastname;
    public string $role_name;  // Not set when query server info
    public string $password;   // Not set, only when create/modify user
    public bool $admin;
}

class Error
{
    public string $error_msg { get; }
    public int $error_code { get; }
    public int $error_code_long { get; }
}

class Db_Info
{
    public int $reads { get; }
    public int $writes { get; }
    public int $fetches { get; }
    public int $marks { get; }

    public int $page_size { get; }
    public int $num_buffers { get; }
    public int $current_memory { get; }
    public int $max_memory { get; }

    public int $allocation { get; }
    public int $attachment_id { get; }
    public int $read_seq_count { get; }
    public int $read_idx_count { get; }
    public int $insert_count { get; }
    public int $update_count { get; }
    public int $delete_count { get; }
    public int $backout_count { get; }
    public int $purge_count { get; }
    public int $expunge_count { get; }

    public array $isc_version { get; }
    public array $firebird_version { get; }
    public array $limbo { get; }

    public int $sweep_interval { get; }
    public int $ods_version { get; }
    public int $ods_minor_version { get; }
    public int $no_reserve { get; }
}

class Blob_Info
{
    public int $num_segments { get; }
    public int $max_segment { get; }
    public int $total_length { get; }
    public int $type { get; }
}

class Var_Info
{
    public string $name { get; }
    public string $alias { get; }
    public string $relation { get; }
    public int $byte_length { get; }
    public int $type { get; }
    public int $sub_type { get; }
    public int $scale { get; }
    public bool $nullable { get; }
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

    public string $sqlstate { get; }     // SQLSTATE codes are standard five-character error codes defined by SQL standards
}

// There is no internal trait. Used for stub only
trait Fb_Error
{
    protected(set) string $error_msg;
    protected(set) int $error_code;
    protected(set) int $error_code_long;

    protected(set) string $error_file;
    protected(set) int $error_lineno;

    protected(set) array $errors;        // array of all Firebird errors

    protected(set) string $sqlstate;
}

class Connect_Args
{
    var string $database;
    var string $user_name;
    var string $password;
    var string $role_name;
    var string $charset;
    var int $num_buffers;
    var int $timeout;
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
    var int $timeout;
}

class Service_Connect_Args
{
    var string $service_name;
    var string $user_name;
    var string $password;
}

class Database implements IError
{
    use Fb_Error;

    protected(set) Connect_Args|Create_Args $args;

    /** @return bool */
    function connect(Connect_Args $args) {}

    /** @return bool */
    function create(Create_Args $args) {}

    /** @return bool */
    function drop() {}

    /** @return Db_Info|false */
    function get_info() {}

    /** @return bool */
    function on_event(string $name, callable $f) {}

    /** @return Transaction */
    function new_transaction(?TBuilder $tb = null) {}

    /** @return Transaction|false */
    function reconnect_transaction(int $id) {}

    /** @return bool */
    function disconnect() {}

    /** @return int[]|false */
    function get_limbo_transactions(int $max_count) {}
}

// TODO: auto commit/rollback flag?
class Transaction implements IError
{
    use Fb_Error;

    protected(set) int $id;
    protected(set) Database $database;
    protected(set) TBuilder $builder;

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

    /** @return bool */
    function prepare_2pc() {}
}

class Statement implements IError
{
    use Fb_Error;

    protected(set) Transaction $transaction;
    protected(set) string $name;
    protected(set) int $num_vars_in;
    protected(set) int $num_vars_out;
    protected(set) int $insert_count;
    protected(set) int $update_count;
    protected(set) int $delete_count;
    protected(set) int $affected_count; // insert + update + delete

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

    /** @return Var_Info|false */
    function get_var_info_in(int $num) {}

    /** @return Var_Info|false */
    function get_var_info_out(int $num) {}

    /** @return bool */
    function set_name(string $name) {}
}

class Blob implements IError
{
    use Fb_Error;

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

class Event implements IError
{
    use Fb_Error;
    private function __construct() {}

    /** @return bool */
    static function consume() {}
}

class Service implements IError
{
    use Fb_Error;

    protected(set) Service_Connect_Args $args;

    /** @return bool */
    function connect(Service_Connect_Args $args) {}

    /** @return bool */
    function disconnect() {}

    /** @return Server_Info|false */
    function get_server_info() {}

    /** @return bool */
    function add_user(Server_User_Info $user_info) {}

    /** @return bool */
    function modify_user(Server_User_Info $user_info) {}

    /** @return bool */
    function delete_user(string $username) {}

    /** @return bool */
    function backup(string $dbname, string $bkp_file, int $options = 0) {}

    /** @return bool */
    function restore(string $bkp_file, string $dbname, int $options = 0) {}

    // function db_info() {}
    // function server_info() {}
}

/**
 * Transaction builder
 */
class TBuilder
{
    readonly bool $is_read_only;
    readonly bool $is_ignore_limbo;
    readonly bool $is_auto_commit;
    readonly bool $is_no_auto_undo;
    readonly int $isolation_mode;
    readonly int $lock_timeout;
    readonly int $snapshot_at_number;

    /** @return static  */
    function read_only(bool $enable = true) {}

    /** @return static  */
    function ignore_limbo(bool $enable = true) {}

    /** @return static  */
    function auto_commit(bool $enable = true) {}

    /** @return static  */
    function no_auto_undo(bool $enable = true) {}

    /**
     * Lock resolution mode (WAIT, NO WAIT) with an optional LOCK TIMEOUT specification
     *
     * @param int $lock_timeout -1 wait forever (default), 0 no wait, 1-32767 sets lock timeout in seconds
     * @return static
     */
    function wait(int $lock_timeout = -1) {}

    // | RESTART REQUESTS // The exact semantics and effects of this clause are not clear, and we recommend you do not use this clause.

    // TODO: function reserving(array $tables) {}

    /**
     * Isolation Level SNAPSHOT - the default level. Also known as "concurrency"
     *
     * @return static
     */
    function isolation_snapshot(int $at_number = 0) {}

    /** @return static  */
    function isolation_snapshot_table_stability() {}

    /** @return static  */
    function isolation_read_committed_record_version() {}

    /** @return static  */
    function isolation_read_committed_no_record_version() {}

    /** @return static  */
    function isolation_read_committed_read_consistency() {}

    function dump_state(): void {}
}
