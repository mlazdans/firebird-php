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
    $t = $db->start_transaction();

    $table = "TEST_001";
    $fields = "BLOB_1, DECFLOAT_16, DECFLOAT_34";

    $queries = [
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

    query_bulk($t, $queries);

    $t->start();
    $q = $t->query("SELECT BLOB_1, DECFLOAT_16, DECFLOAT_34, BOOL_1 FROM $table");
    fetch_and_print($q, \FireBird\FETCH_BLOB_TEXT);
})();

?>
--EXPECT_EXTERNAL--
008-exec_immed.out.txt
