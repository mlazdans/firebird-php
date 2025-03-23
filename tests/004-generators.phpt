--TEST--
FireBird: generators
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

    if(false === ($t = $conn->new_transaction())) {
        print_error_and_die("transaction", $conn);
    }

    $queries = [
        "SET TRANSACTION",
        load_file_or_die(Config::$pwd."/001-table.sql"),
        "COMMIT RETAIN",
        "CREATE SEQUENCE GEN_GEN START WITH 1024 INCREMENT BY 8",
        "SET GENERATOR GEN_GEN TO 512",
        "COMMIT RETAIN",
    ];

    query_bulk_or_die($t, $queries);

    query_and_fetch_and_print_or_die($t, 'SELECT NEXT VALUE FOR GEN_GEN FROM RDB$DATABASE', \FireBird\FETCH_BLOBS);
    query_and_fetch_and_print_or_die($t, 'SELECT NEXT VALUE FOR GEN_GEN FROM RDB$DATABASE', \FireBird\FETCH_BLOBS);

    $t->commit();
})();

?>
--EXPECT--
object {
  ["NEXT_VALUE"]=>
  int(520)
}
object {
  ["NEXT_VALUE"]=>
  int(528)
}