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

    function __construct(
        protected Connection $db,
        protected ?int $trans_args = null,
        protected ?int $lock_timeout = null,
    ) {}

    function commit() {}
    function commit_ret() {}
    function rollback() {}
    function rollback_ret() {}
    // function query(string $query, ...$bind_args) {}
    // function prepare(string $query) {}
}

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
