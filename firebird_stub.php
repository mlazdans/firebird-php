<?php declare(strict_types = 1);

namespace FireBird;

const FETCH_BLOBS    = 1;
const FETCH_ARRAYS   = 2;
const FETCH_UNIXTIME = 4;

// It's actually not a trait internally
trait Error {
    protected(set) string $error_msg;
    protected(set) int $error_code;
    protected(set) int $error_code_long;
}

class Database
{
    use Error;
    function __construct(
        protected string $database,
        protected ?string $username = null,
        protected ?string $password = null,
        protected ?string $charset = null,
        protected ?int $buffers = null,
        protected ?int $dialect = null,
        protected ?string $role = null,
    ) {}
    // function drop_db() {}

    // /** @return Connection | false */
    // function connect() {}

    // /** @return bool */
    // function disconnect() {}

    // /** @return bool */
    // function create() {}

    // /** @return bool */
    // function drop() {}
}

class Connection
{
    use Error;

    function __construct(
        protected Database $database
    ) {}

    /**
     * @param ?int $lock_timeout - sets lock timeout in seconds when WAIT | LOCK_TIMEOUT $trans_args are set. Valid range 1-32767
     * @return Transaction | false
     * */
    // function start_transaction(int $trans_args = 0, int $lock_timeout = 0) {}

    /** @return bool */
    function connect() {}

    /** @return bool */
    function disconnect() {}
}

// TODO: auto commit/rollback flag?
class Transaction {
    use Error;

    /**
     * @param ?int $lock_timeout set lock timeout in seconds when $trans_args are set to WAIT | LOCK_TIMEOUT. Valid range 1-32767
     */
    function __construct(
        protected Connection $connection,
        protected ?int $trans_args = null,
        protected ?int $lock_timeout = null,
    ) {}

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
}

class Statement {
    use Error;

    function __construct(
        protected Transaction $transaction
    ) {}

    /** @return Statement | false */
    function query(string $sql, ...$bind_args) {}

    /** @return Statement | false */
    function prepare(string $sql) {}

    /** @return object | false | null */
    function fetch_object(int $flags = 0) {}

    /** @return array | false | null */
    function fetch_array(int $flags = 0) {}

    /** @return array | false | null */
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
