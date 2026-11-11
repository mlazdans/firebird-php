--TEST--
Duplicate and empty column aliases get unique keys, also past 99 duplicates
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db("", true, false)->start_transaction();

    $cols = implode(", ", array_map(fn($i) => "$i AS A", range(1, 102)));
    $row = $t->query("SELECT $cols FROM RDB\$DATABASE")->fetch_array();
    $keys = array_keys($row);
    var_dump(count($row), $keys[0], $keys[1], $keys[99], $keys[100], $keys[101], $row['A_101']);

    // An alias that collides with a generated name, and expressions without a name
    $row = $t->query("SELECT 1 AS A, 2 AS A, 3 AS A_01, 4 + 0, 5 + 0 FROM RDB\$DATABASE")->fetch_array();
    var_dump(array_keys($row));

    $t->commit();
})();
?>
--EXPECT--
int(102)
string(1) "A"
string(4) "A_01"
string(4) "A_99"
string(5) "A_100"
string(5) "A_101"
int(102)
array(5) {
  [0]=>
  string(1) "A"
  [1]=>
  string(4) "A_01"
  [2]=>
  string(7) "A_01_01"
  [3]=>
  string(3) "ADD"
  [4]=>
  string(6) "ADD_01"
}
