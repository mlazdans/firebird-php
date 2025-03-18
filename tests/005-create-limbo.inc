<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $share_path = sys_get_temp_dir().DIRECTORY_SEPARATOR."firebird-php-005-db.txt";

    if(file_exists($share_path)) {
        unlink($share_path);
        // die("skip File $database_share_path already exists. Manual cleanup recommended.");
    }

    $conn = init_tmp_db(drop: false);

    $tr_id = (function() use ($conn): int {
        // Create table
        $t = $conn->new_transaction()                              or print_error_and_die("new_transaction", $conn);
        $t->start()                                                or print_error_and_die("transaction start", $t);
        $t->query(load_file_or_die(Config::$pwd."/001-table.sql")) or print_error_and_die("create table", $t);
        $t->commit_ret()                                           or print_error_and_die("commit_ret", $t);

        // First insert some data via standard transaction
        $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Bogus mogus") or print_error_and_die("query", $t);
        $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Foo bar")     or print_error_and_die("query", $t);
        $t->commit()                                                         or print_error_and_die("commit", $t);

        // Insert more data and put transaction in limbo
        $t = $conn->new_transaction()                                                          or print_error_and_die("new_transaction2", $conn);
        $t->start()                                                                            or print_error_and_die("transaction start", $t);
        $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Hello from transaction $t->id") or print_error_and_die("query", $t);
        $t->prepare_2pc()                                                                      or print_error_and_die("prepare_2pc", $t);

        print "Transaciton ID: $t->id prepared for two-phase commit\n";

        // These both inserts should fail because transaction has been prepared for 2pc
        if($t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Hello from insert after 2pc transaction $t->id")) {
            die("No insert after prepare_2pc should happen!");
        }

        if($p = $t->prepare("INSERT INTO TEST_001 (BLOB_1) VALUES (?)")) {
            if($p->execute("Hello from prepare after 2pc transaction $t->id")) {
                die("No prepare after prepare_2pc should happen!");
            }
            $p->free();
        }

        return $t->id;
    })();

    file_put_contents($share_path, $conn->args->database."\n$tr_id");
})();
