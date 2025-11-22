--TEST--
FireBird: basic operations
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

// \FireBird\set_error_handler(function(...$args){
//     print_r($args);
//     die('ERROR');
// });

(function(){
    $conn = init_tmp_db();
    $t = $conn->start_transaction() or print_error_and_die("start_transaction");

    exec_from_file_ddl($t, "001-table.sql");

    $table = "TEST_001";

    $insert_tests = [
        ["Insert DECFLOAT inline",
            "INSERT INTO $table (ID, DECFLOAT_16, DECFLOAT_34) VALUES (DEFAULT, 3.141592653589793, 3.141592653589793238462643383279502) RETURNING ID", [], \FireBird\FETCH_FETCH_BLOB_TEXT],
        ["Insert DECFLOAT as bind arguments",
            "INSERT INTO $table (DECFLOAT_16, DECFLOAT_34) VALUES (?, ?) RETURNING ID", ["3.141592653589793", "3.141592653589793238462643383279502"], 0],
    ];

    run_tests($t, $insert_tests);

    $select_tests = [
        ["Select all fields",
            "SELECT * FROM $table", [], \FireBird\FETCH_BLOB_TEXT],
        ["Select DECFLOAT_34 where condition inline",
            "SELECT DECFLOAT_34 FROM $table WHERE DECFLOAT_34 = 3.141592653589793238462643383279502", [], 0],
        ["Select DECFLOAT_34 where condition bind",
            "SELECT DECFLOAT_34 FROM $table WHERE DECFLOAT_34 = ?", ["3.141592653589793238462643383279502"], 0],
        ["Select DECFLOAT_16 where condition inline",
            "SELECT DECFLOAT_16 FROM $table WHERE DECFLOAT_16 = 3.141592653589793", [], 0],
        ["Select DECFLOAT_16 where condition bind",
            "SELECT DECFLOAT_16 FROM $table WHERE DECFLOAT_16 = ?", ["3.141592653589793"], 0],

    ];
    if ($results = run_tests($t, $select_tests)) {
        printf("Test1 == Test2 == true? %s\n", results_equal($results[1], $results[2]) ? "YES" : "NO");
        printf("Test3 == Test4 == true? %s\n", results_equal($results[3], $results[4]) ? "YES" : "NO");
    }

    $t->commit() or print_error_and_die("commit");
})();

?>
--EXPECT_EXTERNAL--
001.out.txt
