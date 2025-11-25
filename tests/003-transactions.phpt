--TEST--
FireBird: transactions
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db();
    $t = $db->new_transaction();

    $table = "TEST_001";

    $queries = [
        "SET TRANSACTION",
        empty($db_file) ? load_file_or_die(Config::$pwd."/001-table.sql") : 'SELECT * FROM RDB$DATABASE',
        "COMMIT RETAIN",
        "DELETE FROM $table",
        "COMMIT RETAIN",
        "INSERT INTO $table (BLOB_1) VALUES ('This should be inserted')",
        "SAVEPOINT sp_name",
        "INSERT INTO $table (BLOB_1) VALUES ('This should be deleted')",
        "ROLLBACK TO SAVEPOINT sp_name",
        "COMMIT",
    ];

    execute_immediate_bulk($t, $queries);

    $t->execute("SET TRANSACTION");
    $q = $t->query("SELECT BLOB_1 FROM $table");
    fetch_and_print($q, \FireBird\FETCH_FETCH_BLOB_TEXT);
    $t->execute("COMMIT");

    // $builder = (new \FireBird\TBuilder)->read_only();
    // $t = new \FireBird\Transaction($db, $builder);
    // $t->start() or print_error_and_die("start", $t);
    // $t->query("INSERT INTO $table (BLOB_1) VALUES (?)", "Read only transaction can't insert anything") or print "$t->error_msg\n";
    // $t->rollback() or print_error_and_die("commit", $t);

    // $t1 = $db->new_transaction(
    //     (new \FireBird\TBuilder)
    //     // ->wait(10)
    //     ->isolation_snapshot_table_stability()
    // );

    // $t2 = $db->new_transaction(
    //     (new \FireBird\TBuilder)
    //     // ->wait(10)
    //     ->isolation_snapshot_table_stability()
    // );

    // // "Insert from isolation_snapshot_table_stability"
    // for($step = 1; $step < 10; $step++){
    //     if($step == 1) {
    //         $t1->start() or print "$t1->error_msg\n";
    //         $s1 = $t1->query("UPDATE $table SET BLOB_1 = ?", "Update from isolation_snapshot_table_stability") or print "$t1->error_msg\n";
    //     }

    //     if($step == 2) {
    //         $t2->start() or print "$t2->error_msg\n";
    //         $s2 = $t2->query("SELECT ID, BLOB_1 FROM $table") or print "$t2->error_msg\n";
    //         var_dump($s2);

    //         $r2 = $s2->fetch_object(\FireBird\FETCH_BLOBS);
    //         if($r2 === false) print_error("fetch", $s2);
    //         var_bin($r2);
    //     }

    //     // if($step == 3) {
    //     //     $t2->commit();
    //     // }

    //     // if($step == 3) {
    //     //     $t1->rollback();
    //     // }

    //     if($step == 4) {
    //         fetch_and_print($s1, \FireBird\FETCH_BLOBS);
    //     }

    //     if($step == 5) {
    //         fetch_and_print($s2, \FireBird\FETCH_BLOBS);
    //     }
    // }
    // $t1->commit();
    // $t2->commit();

    // $t3 = $db->new_transaction();
    // $t3->start() or print_error_and_die("start", $t3);
    // $q3 = query_or_die($t3, "SELECT BLOB_1 FROM $table");
    // fetch_and_print_or_die($q3, \FireBird\FETCH_BLOBS);
})();

?>
--EXPECT--
object(stdClass) {
  ["BLOB_1"]=>
  string(23) "This should be inserted"
}