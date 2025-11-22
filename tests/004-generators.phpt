--TEST--
FireBird: generators
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db();
    $t = $db->new_transaction();

    $queries = [
        "SET TRANSACTION",
        load_file_or_die(Config::$pwd."/001-table.sql"),
        "COMMIT RETAIN",
        "CREATE SEQUENCE GEN_GEN START WITH 1024 INCREMENT BY 8",
        "SET GENERATOR GEN_GEN TO 512",
        "COMMIT RETAIN",
    ];

    execute_immediate_bulk_or_die($t, $queries);

    query_and_fetch_and_print_or_die($t, 'SELECT NEXT VALUE FOR GEN_GEN FROM RDB$DATABASE', \FireBird\FETCH_BLOB_TEXT);
    query_and_fetch_and_print_or_die($t, 'SELECT NEXT VALUE FOR GEN_GEN FROM RDB$DATABASE', \FireBird\FETCH_BLOB_TEXT);

    $t->commit() or print_error_and_die("commit");
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