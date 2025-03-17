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

    $tr_id = (function() use ($conn): int {
        $t = $conn->new_transaction() or print_error_and_die("new_transaction", $conn);
        $t->start() or print_error_and_die("transaction start", $t);
        $t->query(load_file_or_die(Config::$pwd."/001-table.sql")) or print_error_and_die("create table", $t);
        $t->commit();

        $t = $conn->new_transaction() or print_error_and_die("new_transaction2", $conn);
        $t->start() or print_error_and_die("transaction start", $t);
        $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Hello from transaction $t->id") or print_error_and_die("query", $t);
        $t->prepare_2pc() or print_error_and_die("prepare_2pc", $t);

        if($t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Hello from insert after 2pc transaction $t->id")) {
            die("No insert after prepare_2pc should happen!");
        }

        if($p = $t->prepare("INSERT INTO TEST_001 (BLOB_1) VALUES (?)")) {
            if($p->execute("Hello from prepare after 2pc transaction $t->id")) {
                die("No prepare after prepare_2pc should happen!");
            }
        }

        return $t->id;
    })();

    $printer = function() use ($args) {
        $db = new \FireBird\Database();
        $conn = $db->connect($args) or print_error_and_die("2nd connection", $db);
        $t = $conn->new_transaction() or print_error_and_die("new_transaction", $conn);
        $t->start() or print_error_and_die("transaction start", $t);
        $q = $t->query("SELECT BLOB_1 FROM TEST_001");
        fetch_and_print_or_die($q, \FireBird\FETCH_BLOBS);
        $q->free() or print_error_and_die("free", $q);
        $t->commit() or print_error_and_die("commit", $t);
        $conn->disconnect() or print_error_and_die("disconnect", $conn);
    };

    $reconnecter = function(int $tr_id) use ($args) {
        $db = new \FireBird\Database();
        $conn = $db->connect($args) or print_error_and_die("3rd connection", $db);
        $t = $conn->reconnect_transaction($tr_id) or print_error_and_die("reconnect_transaction", $conn);
        $t->commit() or print_error_and_die("commit", $t);
        $conn->disconnect() or print_error_and_die("disconnect3", $conn);
    };

    $printer(); // Should not print anything
    $reconnecter($tr_id);
    $printer(); // Now should print "Hello from transaction %d"

    // Test bogus
    $reconnecter(999);
})();

?>
--EXPECTF--
object {
  ["BLOB_1"]=>
  string(%d) "Hello from transaction %d"
}
===============================================================================
ERROR [reconnect_transaction]:[HY000] %s
0: 335544468 (-901) transaction is not in limbo
1: 335544468 (-901) transaction 999 is in an ill-defined state
===============================================================================