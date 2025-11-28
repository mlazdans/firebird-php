--TEST--
FireBird: generators
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db(init_default_tables:false)->new_transaction();

    $queries = [
        "SET TRANSACTION",
        load_file_or_die(Config::$pwd."/001-TEST_001.sql"),
        "COMMIT RETAIN",
        "CREATE SEQUENCE GEN_GEN START WITH 1024 INCREMENT BY 8",
        "SET GENERATOR GEN_GEN TO 512",
        "COMMIT RETAIN",
    ];

    execute_immediate_bulk($t, $queries);

    query_and_fetch_and_print($t, 'SELECT NEXT VALUE FOR GEN_GEN FROM RDB$DATABASE', \FireBird\FETCH_BLOB_TEXT);
    query_and_fetch_and_print($t, 'SELECT NEXT VALUE FOR GEN_GEN FROM RDB$DATABASE', \FireBird\FETCH_BLOB_TEXT);

    $t->commit();
})();

?>
--EXPECT--
object(stdClass) {
  ["NEXT_VALUE"]=>
  int(520)
}
object(stdClass) {
  ["NEXT_VALUE"]=>
  int(528)
}