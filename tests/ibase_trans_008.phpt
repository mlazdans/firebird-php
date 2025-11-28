--TEST--
php-firebird: transaction control with SQL
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $queries = [
        "SET TRANSACTION",
        "DELETE FROM TEST1",
        "COMMIT RETAIN",
        ["INSERT INTO TEST1 (I, C) VALUES (?, ?)", [1, "test1(1)"]],
        "SAVEPOINT sp_name",
        ["INSERT INTO TEST1 (I, C) VALUES (?, ?)", [2, "test1(2)"]],
        "ROLLBACK TO SAVEPOINT sp_name",
        "COMMIT",
    ];
    $db = init_tmp_db();
    $t = $db->new_transaction();
    query_bulk($t, $queries);

    $t = $db->start_transaction();
    dump_table_rows($t, "TEST1");
})();

?>
--EXPECT--
array(2) {
  ["I"]=>
  int(1)
  ["C"]=>
  string(8) "test1(1)"
}