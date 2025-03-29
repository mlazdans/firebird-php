--TEST--
FireBird: two-phase commit
--SKIPIF--
<?php declare(strict_types = 1);

// Hacky way to simulate limbo transaction. In separate process create
// transaction, prepare and then just exit. Share created database path and
// transaction id with main test

namespace FireBirdTests;

include("skipif.inc");

$php_ini_path = realpath(__DIR__.DIRECTORY_SEPARATOR.'..');

$cmd = $_ENV['TEST_PHP_EXECUTABLE']." -c '$php_ini_path' -d 'firebird.debug=0' ".__DIR__.DIRECTORY_SEPARATOR."005-create-limbo.inc";
if((false === exec($cmd, $output, $result)) || $result) {
    "skip Could not execute limbo transaction creating in separate process";
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
        $conn = new \FireBird\Connector;
        $db = $conn->connect($args)                   or print_error_and_die("2nd connection", $conn);
        $t = $db->new_transaction()                 or print_error_and_die("new_transaction", $db);
        $t->start()                                   or print_error_and_die("transaction start", $t);
        $q = $t->query("SELECT BLOB_1 FROM TEST_001") or print_error_and_die("query", $t);

        fetch_and_print($q, \FireBird\FETCH_BLOBS);

        $q->free()                                    or print_error_and_die("free", $q);
        $t->commit()                                  or print_error_and_die("commit", $t);
        $db->disconnect()                           or print_error_and_die("disconnect2", $db);
    };

    $get_limbo_array = function() use ($args) {
        $conn = new \FireBird\Connector;
        $db = $conn->connect($args) or print_error_and_die("limbo connection", $conn);

        if(false === ($limbo = $db->get_limbo_transactions(100))) {
            print_error_and_die("get_limbo_transactions", $db);
        }

        printf("Limbo transaction count: %d, expected count: %d\n", count($limbo), 1);
        $db->disconnect() or print_error_and_die("disconnect limbo", $db);
    };

    $get_limbo_array();

    $reconnecter = function(int $tr_id) use ($args) {
        $conn = new \FireBird\Connector;

        $db = $conn->connect($args)               or print_error_and_die("3rd connection", $conn);
        $t = $db->reconnect_transaction($tr_id) or print_error_and_die("reconnect_transaction", $db);
        $t->commit()                              or print_error_and_die("commit", $t);
        $db->disconnect()                       or print_error_and_die("disconnect3", $db);
    };

    hl("Step 1");
    $printer(); // Should not print only two inserted rows and error about transaction in limbo

    hl("Step 2");
    $reconnecter($tr_id);
    $printer(); // Now should print two inserted rows + row with "Hello from transaction %d"

    // Test reconnect to a bogus transaction
    (function(int $tr_id) use ($args) {
        $conn = new \FireBird\Connector;

        $db = $conn->connect($args)               or print_error_and_die("4th connection", $conn);
        if(false === $db->reconnect_transaction($tr_id)) {
            print_error("reconnect_transaction", $db);
        } else {
            user_error("reconnect_transaction with bogus transaction should not succeed!", E_USER_ERROR);
        }
        $db->disconnect()                       or print_error_and_die("disconnect3", $db);
    })(999);

    // Drop
    $conn = new \FireBird\Connector;
    $db = $conn->connect($args)  or print_error_and_die("4th connection", $conn);
    $db->drop()          or print_error_and_die("drop", $db);

    unlink($share_path);
})();

?>
--EXPECTF_EXTERNAL--
005.out.txt
