--TEST--
Transaction tests
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    if(false === ($conn = init_tmp_db())) {
        return;
    }

    print "Connected\n";

    if(false === ($t = $conn->new_transaction())) {
        print_error_and_die("transaction", $conn);
    }

    $table = "TEST_001";

    $queries = [
        "SET TRANSACTION",
        load_file_or_die(Config::$pwd."/001-table.sql"),
        "COMMIT RETAIN",
        ["INSERT INTO $table (BLOB_1) VALUES (?)", ["This should be inserted"]],
        "SAVEPOINT sp_name",
        ["INSERT INTO $table (BLOB_1) VALUES (?)", ["This should be deleted"]],
        "ROLLBACK TO SAVEPOINT sp_name",
        "COMMIT",
    ];

    query_bulk_or_die($t, $queries);

    query_or_die($t, "SET TRANSACTION");
    $q = query_or_die($t, "SELECT BLOB_1 FROM $table");
    fetch_and_print_or_die($q, \FireBird\FETCH_BLOBS);
})();

?>
--EXPECT_EXTERNAL--
003.out.txt
