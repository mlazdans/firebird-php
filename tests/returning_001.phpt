--TEST--
php-firebird: INSERT / UPDATE using RETURNING
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db()->start_transaction();

    $t->query("DELETE FROM TEST1");

    $q = $t->query("INSERT INTO TEST1 (I, C) VALUES (?, ?) RETURNING I, C", 1, "data 1");
    dump_rows($q);
    dump_table_rows($t, "TEST1");

    print "--------\n";

    $q = $t->query("UPDATE TEST1 SET I = I + 1, C = C || ' updated' RETURNING OLD.I AS OLD_I, OLD.C AS OLD_C, NEW.I AS NEW_I, NEW.C AS NEW_C");
    dump_rows($q);
    dump_table_rows($t, "TEST1");
})();

?>
--EXPECT--
array(2) {
  ["I"]=>
  int(1)
  ["C"]=>
  string(6) "data 1"
}
array(2) {
  ["I"]=>
  int(1)
  ["C"]=>
  string(6) "data 1"
}
--------
array(4) {
  ["OLD_I"]=>
  int(1)
  ["OLD_C"]=>
  string(6) "data 1"
  ["NEW_I"]=>
  int(2)
  ["NEW_C"]=>
  string(14) "data 1 updated"
}
array(2) {
  ["I"]=>
  int(2)
  ["C"]=>
  string(14) "data 1 updated"
}