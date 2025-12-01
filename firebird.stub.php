<?php

/** @generate-class-entries */
/** @generate-function-entries */
/** @tentative-return-type */
/** @generate-legacy-arginfo 80400 */

namespace FireBird;
/**
 * fetch to return blob fields as strings
 * @var int
 * @cvalue FBP_FETCH_BLOB_TEXT
 */
const FETCH_BLOB_TEXT = UNKNOWN;

/**
 * fetch to return date fields as UNIX timestamps
 * @var int
 * @cvalue FBP_FETCH_UNIXTIME
 */
const FETCH_UNIXTIME = UNKNOWN;

/**
 * fetch to return date/time fields as PHP DateTime objects
 * @var int
 * @cvalue FBP_FETCH_DATE_OBJ
 */
const FETCH_DATE_OBJ = UNKNOWN;

/**
 * fetch to return blob fields as Blob objects
 * @var int
 * @cvalue FBP_FETCH_BLOB_TEXT
 */
const FETCH_FETCH_BLOB_TEXT = UNKNOWN;

/**
 * Option to check if Used Blob->type is segmented.
 * @see Blob::$type
 * @var int
 * @cvalue FBP_BLOB_TYPE_SEGMENTED
 */
const BLOB_TYPE_SEGMENTED = UNKNOWN;

/**
 * Option to check if Used Blob->type is streamed.
 * @see Blob::$type
 * @var int
 * @cvalue FBP_BLOB_TYPE_STREAMED
 */
const BLOB_TYPE_STREAMED = UNKNOWN;

/**
 * Option to seek blob from start position. Applies only on streamed blob type.
 * @see Blob::seek()
 * @var int
 * @cvalue FBP_BLOB_SEEK_START
 */
const BLOB_SEEK_START = UNKNOWN;

/**
 * Option to seek blob from current position. Applies only on streamed blob type.
 * @see Blob::seek()
 * @var int
 * @cvalue FBP_BLOB_SEEK_CURRENT
 */
const BLOB_SEEK_CURRENT = UNKNOWN;

/**
 * Option to seek blob from end position. Applies only on streamed blob type.
 * @see Blob::seek()
 * @var int
 * @cvalue FBP_BLOB_SEEK_END
 */
const BLOB_SEEK_END = UNKNOWN;

/**
 * @see Service::shutdown_db()
 * @var int
 */
const SM_NORMAL = 0;

/**
 * @see Service::shutdown_db()
 * @var int
 */
const SM_MULTI  = 1;

/**
 * @see Service::shutdown_db()
 * @var int
 */
const SM_SINGLE = 2;

/**
 * @see Service::shutdown_db()
 * @var int
 */
const SM_FULL   = 3;

function get_client_version(): float { die; }
function get_client_major_version(): int { die; }
function get_client_minor_version(): int { die; }

// function get_errors(): array { die; }
// function set_error_handler(callable $handler): void {}

// abstract class Error_Handler
// {
//     abstract function on_error(array $errors);
// }

// interface IError
// {
//     public readonly string $error_msg;
//     public readonly int $error_code;
//     public readonly int $error_code_long;
//     public readonly string $error_file;
//     public readonly int $error_lineno;
//     public readonly array $errors;
//     public readonly string $sqlstate;
// }

/**
 * This interface is used throughout most of FireBird classes to keep track all errors.
 * There is no internal trait. Used for stub only.
 */
// trait Fb_Error
// {
//     /**
//      * Combined multi-line error message from errors array.
//      * @see $errors
//      */
//     public string $error_msg;

//     /**
//      * Most recent error code
//      */
//     public int $error_code;

//     /**
//      * Most recent long error code
//      */
//     public int $error_code_long;

//     /**
//      * PHP file where error occurred
//      */
//     public string $error_file;

//     /**
//      * PHP line number where error occurred
//      */
//     public int $error_lineno;

//     /**
//      * Firebird can report multiple errors and arror codes at the same time.
//      * Array of Error object of all errors in order of appearance
//      *
//      * @var Error[]
//      */
//     public array $errors;

//     /**
//      * Stores SQLSTATE code.
//      *
//      * The structure of an SQLSTATE error code is five characters comprising the
//      * SQL error class (2 characters) and the SQL subclass (3 characters).
//      *
//      * Although Firebird tries to use SQLSTATE codes defined in ISO/IEC 9075
//      * (the SQL standard), some are non-standard or derive from older standards
//      * like X/Open SQL for historic reasons.
//      *
//      * @link https://firebirdsql.org/file/documentation/html/en/refdocs/fblangref50/firebird-50-language-reference.html#fblangref50-appx02-sqlstates
//      *
//      */
//     public string $sqlstate;
// }

class Database
{
    public readonly Connect_Args|Create_Args|null $args;

    /**
     * Do not instantiate this class directly. Use Database::connect() or
     * Database::create() or Database::execute_create() instead.
     */
    private function __construct() {}

    /**
     * @throws Fb_Exception
     */
    static public function connect(Connect_Args $args): Database { die; }

    /**
     * @throws Fb_Exception
     */
    static public function create(Create_Args $args): Database { die; }

    /**
     * @throws Fb_Exception
     */
    static public function execute_create(string $sql): Database { die; }

    /**
     * Wrapper around (new Transaction($this))->start(?TBuilder $tb = null);
     * @throws Fb_Exception
     */
    public function start_transaction(?TBuilder $tb = null): Transaction { die; }

    /**
     * Wrapper around new Transaction($this);
     * @throws Fb_Exception
     */
    public function new_transaction(): Transaction { die; }

    /**
     * @throws Fb_Exception
     */
    public function get_info(): Db_Info { die; }

    /**
     * @throws Fb_Exception
     */
    public function disconnect(): void { die; }

    /**
     * @throws Fb_Exception
     */
    public function drop(): void { die; }

    /**
     * Installs event handler. Currently, the user of this function must
     * organize the event loop manually and consume them.
     *
     * Events in Firebird can happend multiple times while PHP is processing
     * other stuff.
     *
     * Simplified example event loop:
     * <code>
     * <?php
     * $db->on_event("TEST1", function(int $count) {
     *     print "Event was posted: $count times\n";
     * });
     *
     * while (true) {
     *     \FireBird\Event::consume();
     * }
     *</code>
    *
    * @param callable $f function that accepts single integer argument (posted
    * event count)
    *
    * */
    public function on_event(string $name, callable $f): bool { die; }

    /**
     * Connect to a transaction in limbo.
     *
     * @see get_limbo_transactions()
     * */
    public function reconnect_transaction(int $id): Transaction|false { die; }

    /** @return int[]|false */
    public function get_limbo_transactions(int $max_count): array|false { die; }
}

// TODO: Temp disable
// class Multi_Transaction
// {
//     public function __construct() {}

//     /**
//      * Adds a database to Multi_Transaction.
//      *
//      * Returns Transaction object you can now query and execute on. Any attempt
//      * to commit/rollback on returned transaction object will result in whole
//      * multi transaction commit/rollback.
//      *
//      * */
//     public function add_db(Database $database, ?TBuilder $builder = null): Transaction { die; }

//     public function start(): bool { die; }
//     public function commit(): bool { die; }
//     public function commit_ret(): bool { die; }
//     public function rollback(): bool { die; }
//     public function rollback_ret(): bool { die; }

//     /**
//      * Prepares multi transaction for two-phase commit.
//      * Optional description will be available in RDB$TRANSACTIONS table.
//      * */
//     public function prepare_2pc(?string $description = null): bool { die; }
// }

// TODO: auto commit/rollback flag?
class Transaction
{
    /** @virtual */
    public readonly int $id;
    public readonly Database $database;

    public function __construct(Database $database) {}
    public function start(?TBuilder $builder = null): void { die; }
    public function commit(): void { die; }
    public function commit_ret(): void { die; }
    public function rollback(): void { die; }
    public function rollback_ret(): void { die; }
    public function query(string $sql, mixed ...$bind_args): Statement { die; }
    public function prepare(string $sql): Statement { die; }
    public function open_blob(Blob_Id $id): Blob { die; }
    public function create_blob(): Blob { die; }

    /**
     * Execute w/o prepare step. Will initialize $this in case of SET TRANSACTION
     */
    public function execute(string $sql): void { die; }

    /**
     * Prepares transaction for two-phase commit.
     * */
    // public function prepare_2pc(): bool { die; }
}

class Statement
{
    /** @virtual */
    public readonly string $name;
    /** @virtual */
    public readonly int $in_vars_count;
    /** @virtual */
    public readonly int $out_vars_count;
    /** @virtual */
    public readonly int $insert_count;
    /** @virtual */
    public readonly int $update_count;
    /** @virtual */
    public readonly int $delete_count;
    /** @virtual */
    public readonly int $affected_count; // insert + update + delete

    public readonly Transaction $transaction;

    private function __construct() {}

    /**
     * Fetch row as object.
     *
     * Returns an object with the row information, false on error or null if there are no more rows
     *
     * @see FETCH_BLOBS, FETCH_UNIXTIME
     * */
    public function fetch_object(int $flags = 0): object|false|null { die; }

    /**
     * Fetch row as associative array.
     *
     * Returns an array with the row information, false on error or null if there are no more rows
     *
     * @see FETCH_BLOBS, FETCH_UNIXTIME
     * */
    public function fetch_array(int $flags = 0): array|false|null { die; }

    /**
     * Fetch row as indexed array.
     *
     * Returns an array with the row information, false on error or null if there are no more rows
     *
     * @see FETCH_BLOBS, FETCH_UNIXTIME
     * */
    public function fetch_row(int $flags = 0): array|false|null { die; }
    public function execute(mixed ...$bind_args): void { die; }
    public function close_cursor(): void { die; }
    public function free(): void { die; }

    /**
     * Gets information about inbound variable from a statement.
     *
     * @see $num_vars_in
     * */
    public function get_var_info_in(int $num): Var_Info|false { die; }

    /**
     * Gets information about outbound variable from a statement.
     *
     * @see $num_vars_out
     * */
    public function get_var_info_out(int $num): Var_Info|false { die; }

    /**
     * Set a cursor name for a dynamic request.
     *
     * To be able to use the WHERE CURRENT OF clause in DSQL, the cursor name
     * needs to be set on the statement handle before executing the statement.
     *
     * SELECT 0 FROM TEST_TABLE WHERE ID < 67 FOR UPDATE;
     *
     * UPDATE TEST_TABLE SET FIELD_1 = ? WHERE CURRENT OF my_cursor_name;
     *
     * */
    public function set_name(string $name): bool { die; }
}

class Blob
{
    /** @virtual */
    public readonly int $num_segments;
    /** @virtual */
    public readonly int $max_segment;
    /** @virtual */
    public readonly int $total_length;
    /** @virtual */
    public readonly int $type;
    /** @virtual */
    public readonly int $position;
    /** @virtual */
    public readonly bool $is_writable;
    public readonly Transaction $transaction;

    public function __construct(\FireBird\Transaction $tr) {}
    public function id(): Blob_Id { die; }
    public function create(): void { die; }
    public function open(Blob_Id $id): void { die; }
    public function close(): void { die; }
    public function cancel(): void { die; }
    public function get(int $max_len = 0): string|false { die; }
    public function put(string $data): void { die; }
    public function seek(int $offset, int $mode): int|false { die; }
}

// TODO: Temp disable
// class Event
// {
//     private function __construct() {}
//     public static function consume(): bool { die; }
// }

// TODO: Temp disable
// class Service
// {
//     protected Service_Connect_Args $args;

//     public function connect(Service_Connect_Args $args): bool { die; }
//     public function disconnect(): bool { die; }
//     public function get_server_info(): Server_Info|false { die; }
//     public function add_user(Server_User_Info $user_info): bool { die; }
//     public function modify_user(Server_User_Info $user_info): bool { die; }
//     public function delete_user(string $username): bool { die; }
//     public function backup(string $dbname, string $bkp_file, int $options = 0): bool { die; }
//     public function restore(string $bkp_file, string $dbname, int $options = 0): bool { die; }

//     /**
//      * Shuts down the database when:
//      *   There are no connections to the database, or
//      *   At the end of the timeout period you specify
//      *
//      * @param string $dbname path to database (on service machine)
//      * @param int $mode shutdown mode
//      * @see SM_NORMAL, SM_MULTI, SM_SINGLE, SM_FULL
//      * @param int $timeout wait timeout in seconds
//      * */
//     public function shutdown_db(string $dbname, int $mode = 0, int $timeout = 0): bool { die; }

//     /**
//      * Bring a shutdown database back online
//      *
//      * @param string $dbname path to database (on service machine)
//      * @param int $mode shutdown mode
//      * @see SM_NORMAL, SM_MULTI, SM_SINGLE, SM_FULL
//      * */
//     public function db_online(string $dbname, int $mode = 0): bool { die; }
//     public function set_page_buffers(string $dbname, int $buffers): bool { die; }
//     public function set_sweep_interval(string $dbname, int $interval): bool { die; }
//     public function deny_new_attachments(string $dbname): bool { die; }
//     public function deny_new_transactions(string $dbname): bool { die; }
//     public function set_write_mode_async(string $dbname): bool { die; }
//     public function set_write_mode_sync(string $dbname): bool { die; }
//     public function set_access_mode_readonly(string $dbname): bool { die; }
//     public function set_access_mode_readwrite(string $dbname): bool { die; }
//     public function enable_reserve_space(string $database): bool { die; }
//     public function disable_reserve_space(string $database): bool { die; }
//     public function set_sql_dialect(string $database, int $dialect): bool { die; }

//     // isc_spb_prp_nolinger
//     // isc_spb_prp_activate               Activate shadow files. Legacy stuff

//     // isc_spb_prp_replica_mode
//     // #define isc_spb_prp_rm_none			0
//     // #define isc_spb_prp_rm_readonly		1
//     // #define isc_spb_prp_rm_readwrite	2

//     // Repair / fix
//     // case isc_spb_rpr_commit_trans:
//     // case isc_spb_rpr_rollback_trans:
//     // case isc_spb_rpr_recover_two_phase:
//     // case isc_spb_rpr_par_workers:
// }

/**
 * Transaction builder
 */
class TBuilder
{
    /** @virtual */
    public readonly bool $is_read_only;
    /** @virtual */
    public readonly bool $is_ignore_limbo;
    /** @virtual */
    public readonly bool $is_auto_commit;
    /** @virtual */
    public readonly bool $is_no_auto_undo;
    /** @virtual */
    public readonly int $isolation_mode;
    /** @virtual */
    public readonly int $lock_timeout;
    /** @virtual */
    public readonly int $snapshot_at_number;

    public function read_only(bool $enable = true): TBuilder { die; }
    public function ignore_limbo(bool $enable = true): TBuilder { die; }
    public function auto_commit(bool $enable = true): TBuilder { die; }
    public function no_auto_undo(bool $enable = true): TBuilder { die; }

    /**
     * Lock resolution mode (WAIT, NO WAIT) with an optional LOCK TIMEOUT specification
     *
     * @param int $lock_timeout -1 wait forever (default), 0 no wait, 1-32767 sets lock timeout in seconds
     */
    public function wait(int $lock_timeout = -1): TBuilder { die; }

    // | RESTART REQUESTS // The exact semantics and effects of this clause are not clear, and we recommend you do not use this clause.

    // TODO: function reserving(array $tables) {}

    /**
     * Isolation Level SNAPSHOT - the default level. Also known as "concurrency"
     */
    public function isolation_snapshot(int $at_number = 0): TBuilder { die; }
    public function isolation_snapshot_table_stability(): TBuilder { die; }
    public function isolation_read_committed_record_version(): TBuilder { die; }
    public function isolation_read_committed_no_record_version(): TBuilder { die; }
    public function isolation_read_committed_read_consistency(): TBuilder { die; }
    public function dump_state(): void {}
}

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
    public string $password;   // Not set, only when create/modify user
    public bool $admin;
}

class Fb_Error
{
    public readonly string $error_msg;
    public readonly int $error_code;
    public readonly int $error_code_long;
}

class Db_Info
{
    public int $reads;
    public int $writes;
    public int $fetches;
    public int $marks;

    public int $page_size;
    public int $num_buffers;
    public int $current_memory;
    public int $max_memory;

    public int $allocation;
    public int $attachment_id;
    public int $read_seq_count;
    public int $read_idx_count;
    public int $insert_count;
    public int $update_count;
    public int $delete_count;
    public int $backout_count;
    public int $purge_count;
    public int $expunge_count;

    public array $isc_version;
    public array $firebird_version;
    public array $limbo;

    public int $sweep_interval;
    public int $ods_version;
    public int $ods_minor_version;
    public int $no_reserve;

    public int $forced_writes;
    public int $set_page_buffers;
    public int $db_sql_dialect;
    public int $db_read_only;
    public int $db_size_in_pages;

    public int $oldest_transaction;
    public int $oldest_active;
    public int $oldest_snapshot;
    public int $next_transaction;

    public int $creation_date;
    public int $db_file_size;
    public int $pages_used;
    public int $pages_free;

    public int $ses_idle_timeout_db;
    public int $ses_idle_timeout_att;
    public int $ses_idle_timeout_run;
    public int $conn_flags;

    public string $crypt_key;
    public int $crypt_state;
    public int $statement_timeout_db;
    public int $statement_timeout_att;
    public int $protocol_version;
    public string $crypt_plugin;
    public string $wire_crypt;

    public int $next_attachment;
    public int $next_statement;
    public int $db_guid;
    public int $db_file_id;

    public int $replica_mode;
    public string $username;
    public string $sqlrole;
    public int $parallel_workers;
}

class Var_Info
{
    public string $name;
    public string $alias;
    public string $relation;
    public int $byte_length;
    public int $type;
    public int $sub_type;
    public int $scale;
    public bool $nullable;
}

class Blob_Id
{
    private function __construct() {}
    public function __toString(): string { die; }
    public static function from_str(string $id): Blob_Id { die; }
}

class Connect_Args
{
    public string $database;
    public string $user_name;
    public string $password;
    public string $role_name;
    public string $charset;
    public int $num_buffers;
    public int $timeout;
}

class Create_Args
{
    public string $database;
    public string $user_name;
    public string $password;
    public string $set_db_charset;
    public int $sweep_interval;
    public int $set_page_buffers;
    public int $page_size;
    public bool $force_write;
    public bool $overwrite;
    public int $timeout;
}

class Service_Connect_Args
{
    public string $service_name;
    public string $user_name;
    public string $password;
}

final class Fb_Exception extends \Exception
{
    public readonly string $sqlstate;
    public readonly array $errors;
    public readonly string $file_ext;
    public readonly int $line_ext;
}
