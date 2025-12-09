--TEST--
FireBird: two-phase commit
--SKIPIF--
<?php declare(strict_types = 1);

namespace FireBirdTests;

// Hacky way to simulate limbo transaction. In separate process create
// transaction, prepare and then just exit. Share created database path and
// transaction id with main test

include("skipif.inc");

$cmd = $_SERVER['TEST_PHP_EXECUTABLE'] ?? $_SERVER['TEST_PHP_CGI_EXECUTABLE'] ?? false or die("skip TEST_PHP_EXECUTABLE and TEST_PHP_CGI_EXECUTABLE env not set");
$extra_args = $_SERVER['TEST_PHP_EXTRA_ARGS'] ?? "";
$cmd .= " $extra_args -d \"firebird.debug=0\" \"".__DIR__.DIRECTORY_SEPARATOR."005-create-limbo.inc".'"';

if((false === exec($cmd, $output, $result)) || $result) {
    printf("skip Could not execute limbo transaction creating in separate process: %s", join(" ", $output));
}

?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $share_path = sys_get_temp_dir().DIRECTORY_SEPARATOR."firebird-php-005-db.txt";

    if(!file_exists($share_path) ) {
        die("File $share_path does not exist. Limbo transaction creation failed. Check SKIP section.");
    }

    if(!is_readable($share_path) || !is_file($share_path)) {
        die("File $share_path is not readable.");
    }

    if(false === ($f = fopen($share_path, "r"))) {
        die("Could not load file $share_path.");
    }
    $limbo_database_path = trim(fgets($f));
    $tr_id = (int)fgets($f);
    fclose($f);

    $args = new \FireBird\Connect_Args;
    args_apply_defaults($args);
    $args->database = $limbo_database_path;

    $printer = function() use ($args) {
        $db = \FireBird\Database::connect($args);
        $t = $db->start_transaction();
        $q = $t->query("SELECT BLOB_1 FROM TEST_001");

        fetch_and_print($q, \FireBird\FETCH_FETCH_BLOB_TEXT);

        $q->free();
        $t->commit();
        $db->disconnect();
    };

    $get_limbo_array = function() use ($args) {
        $db = \FireBird\Database::connect($args);
        $limbo = $db->get_limbo_transactions(100);

        printf("Limbo transaction count: %d, expected count: %d\n", count($limbo), 1);
        $db->disconnect();
    };

    $get_limbo_array();

    $reconnecter = function(int $tr_id) use ($args) {
        $db = \FireBird\Database::connect($args);
        $t = $db->reconnect_transaction($tr_id);
        $t->commit();
        $db->disconnect();
    };

    hl("Step 1");
    try {
        $printer(); // Should not print only two inserted rows and error about transaction in limbo
    } catch (\FireBird\Fb_Exception $e) {
        print $e->getMessage()."\n";
    }

    hl("Step 2");
    $reconnecter($tr_id);
    $printer(); // Now should print two inserted rows + row with "Hello from transaction %d"

    // Test reconnect to a bogus transaction
    (function(int $tr_id) use ($args) {
        $db = \FireBird\Database::connect($args);
        $db->reconnect_transaction($tr_id);
        $db->disconnect();
    })(999);

    // Drop
    $db = \FireBird\Database::connect($args);
    $db->drop();

    unlink($share_path);
})();

?>
--EXPECTF--
Limbo transaction count: 1, expected count: 1
==================================== Step 1 ====================================
object(stdClass) {
  ["BLOB_1"]=>
  string(11) "Bogus mogus"
}
object(stdClass) {
  ["BLOB_1"]=>
  string(7) "Foo bar"
}
record from transaction %d is stuck in limbo
==================================== Step 2 ====================================
object(stdClass) {
  ["BLOB_1"]=>
  string(11) "Bogus mogus"
}
object(stdClass) {
  ["BLOB_1"]=>
  string(7) "Foo bar"
}
object(stdClass) {
  ["BLOB_1"]=>
  string(24) "Hello from transaction %d"
}
transaction is not in limbo
transaction 999 is in an ill-defined state %s
