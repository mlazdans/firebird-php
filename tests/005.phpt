--TEST--
FireBird: two-phase commit
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $conn = init_tmp_db();

    $args = new \FireBird\Connect_Args;
    args_apply_defaults($args);
    $args->database = $conn->args->database;

    $t = $conn->new_transaction() or print_error_and_die("new_transaction", $conn);
    $t->start() or print_error_and_die("transaction start", $t);
    $t->query(load_file_or_die(Config::$pwd."/001-table.sql")) or print_error_and_die("create table", $t);
    $t->commit();

    $t = $conn->new_transaction() or print_error_and_die("new_transaction2", $conn);
    $t->start() or print_error_and_die("transaction start", $t);
    $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Hello from transaction $t->id") or print_error_and_die("query", $t);
    $t->prepare_2pc() or print_error_and_die("prepare_2pc", $t);

    $tr_id = $t->id;

    $f = function() use ($args) {
        $db2 = new \FireBird\Database();
        $conn2 = $db2->connect($args) or print_error_and_die("2nd connection", $db2);
        $t = $conn2->new_transaction() or print_error_and_die("new_transaction", $conn2);
        $t->start() or print_error_and_die("transaction start", $t);
        $q = $t->query("SELECT BLOB_1 FROM TEST_001");
        fetch_and_print_or_die($q, \FireBird\FETCH_BLOBS);
        $q->free() or print_error_and_die("free", $q);
        $t->commit() or print_error_and_die("commit", $t);
        $conn2->disconnect() or print_error_and_die("disconnect", $conn2);
    };

    // Should not print anything
    $f();

    $db3 = new \FireBird\Database();
    $conn3 = $db3->connect($args) or print_error_and_die("3rd connection", $db3);
    $t = $conn->reconnect_transaction($tr_id);
    $t->commit();
    $conn3->disconnect();

    // Now should print "Hello from transaction %d"
    $f();
})();

?>
--EXPECTF--
object {
  ["BLOB_1"]=>
  string(%d) "Hello from transaction %d"
}