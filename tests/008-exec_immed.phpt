--TEST--
FireBird: isc_dsql_execute_immediate
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
    $fields = "BLOB_1, DECFLOAT_16, DECFLOAT_34";

    $queries = [
        "SET TRANSACTION",
        load_file_or_die(Config::$pwd."/001-table.sql"),
        "COMMIT RETAIN",
        ["INSERT INTO $table ($fields) VALUES (?, 0, 0)", ["blobby"]],
        ["INSERT INTO $table ($fields) VALUES (NULL, ?, NULL)", [2.718281828459045235360287471352]],
        ["INSERT INTO $table ($fields) VALUES (NULL, NULL, ?)", [2.718281828459045235360287471352]],
        ["INSERT INTO $table ($fields) VALUES (NULL, ?, NULL)", ["2.718281828459045235360287471352"]],
        ["INSERT INTO $table ($fields) VALUES (NULL, NULL, ?)", ["2.718281828459045235360287471352"]],
        ["INSERT INTO $table ($fields, BOOL_1) VALUES (?, ?, ?, ?)", [NULL, NULL, NULL, true]],
        ["INSERT INTO $table ($fields, BOOL_1) VALUES (?, ?, ?, ?)", [NULL, NULL, NULL, false]],
        "COMMIT",
    ];

    execute_immediate_bulk_or_die($t, $queries);

    $t->start() or print_error_and_die("tr start", $t);
    $q = query_or_die($t, "SELECT BLOB_1, DECFLOAT_16, DECFLOAT_34, BOOL_1 FROM $table");
    fetch_and_print_or_die($q, \FireBird\FETCH_BLOBS);
})();

?>
--EXPECT_EXTERNAL--
008-exec_immed.out.txt
