<?php declare(strict_types = 1);

namespace FireBird;

// It's actually not a trait internally
trait Error {
    protected(set) string $error_msg;
    protected(set) int $error_code;
    protected(set) int $error_code_long;
}

class Connection
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

    /** @return bool */
    function close() {}

    /** @return bool */
    function connect() {}

    // function new_transaction(int $trans_args): Transaction {
    //     return new Transaction($this, $trans_args);
    // }
}

class Transaction {
    use Error;

    /**
     * @param ?int $lock_timeout set lock timeout in seconds when $trans_args are set to WAIT | LOCK_TIMEOUT. Valid range 1-32767
     */
    function __construct(
        protected Connection $db,
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
    // function query(string $query, ...$bind_args) {}
    // function prepare(string $query) {}
}

namespace FireBird\Transaction;
const WRITE = 1;
const READ = 2;
const COMMITTED = 8;
const CONSISTENCY = 16;
const CONCURRENCY = 4;
const REC_VERSION = 64;
const REC_NO_VERSION = 32;
const NOWAIT = 256;
const WAIT = 128;
const LOCK_TIMEOUT = 512;

// class Query {
//     function execute(...$bind_args) {}
// }

// class Prepared_Query {
// }

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
