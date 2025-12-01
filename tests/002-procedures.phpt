--TEST--
FireBird: procedures
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db()->start_transaction();
    exec_from_file_ddl($t, "002-proc.sql");

    $table = "TEST_001";

    $insert_tests = [
        ["Insert blob returning ID",
            "INSERT INTO $table (ID, BLOB_0, BLOB_1) VALUES (DEFAULT, 3.141592653589793, 3.141592653589793238462643383279502) RETURNING ID", [], 0],
        ["Insert false returning ID",
            "INSERT INTO $table (BOOL_1) VALUES (?) RETURNING ID", [false], 0],
        ["Insert true returning ID",
            "INSERT INTO $table (BOOL_1) VALUES (?) RETURNING ID", [true], 0],
        ["Insert true with emojis returning ID",
            "INSERT INTO $table (BOOL_1, VARCHAR_1) VALUES (?, ?) RETURNING ID", [true, "This will insert some 😀😀😁😀😀😁😁😂🤣"], 0],
    ];
    run_tests($t, $insert_tests);

    $fields = "ID, BLOB_0, BLOB_1, BOOL_1, VARCHAR_1";
    $select_tests = [
        ["Select from table WHERE BOOL_1 = ? (false)",
            "SELECT $fields FROM $table WHERE BOOL_1 = ?", [false], \FireBird\FETCH_BLOB_TEXT],
        ["Select from table WHERE BOOL_1 = false",
            "SELECT $fields FROM $table WHERE BOOL_1 = FALSE", [], \FireBird\FETCH_BLOB_TEXT],

        ["Select from table WHERE BOOL_1 = ? (true)",
            "SELECT $fields FROM $table WHERE BOOL_1 = ?", [true], \FireBird\FETCH_BLOB_TEXT],
        ["Select from table WHERE BOOL_1 = true",
            "SELECT $fields FROM $table WHERE BOOL_1 = TRUE", [], \FireBird\FETCH_BLOB_TEXT],

        ["Select ID from procedure which returns ID if BOOL_1 = true",
            "SELECT * FROM PROC_002_1(?)", [true], \FireBird\FETCH_BLOB_TEXT],
    ];
    $results = run_tests($t, $select_tests);

    printf("Test0 == Test1 == true? %s\n", results_equal($results[0], $results[1]) ? "YES" : "NO");
    printf("Test2 == Test3 == true? %s\n", results_equal($results[2], $results[3]) ? "YES" : "NO");

    $proc_ok = true;
    foreach($results[4] as $i=>$r) {
        if($r->ID != $results[3][$i]->ID){
            $proc_ok = false;
            break;
        }
    }
    printf("Did procedure returned IDs with true? %s\n", $proc_ok ? "YES" : "NO");

    $t->commit();
})();

?>
--EXPECT_EXTERNAL--
002.out.txt
