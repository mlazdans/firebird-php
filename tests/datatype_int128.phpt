--TEST--
php-firebird: Check for data type INT128 (Firebird 4.0 or above)
--SKIPIF--
<?php
include("skipif.inc");
// TODO: should also check if compiled against fblient >= 4.0. Perhaps runtime
// client lib checking also needed.
// TODO: implement services first
// skip_if_fb_lt(4.0);
?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db()->start_transaction();

    $t->query(
        "CREATE TABLE TEST_DT (
            V_INT128 INT128 NOT NULL
         )");
    $t->commit_ret();

    $t->query("INSERT INTO TEST_DT (V_INT128) VALUES (1234)");
    $t->query("INSERT INTO TEST_DT (V_INT128) VALUES (-170141183460469231731687303715884105728)");
    $t->query("INSERT INTO TEST_DT (V_INT128) VALUES (170141183460469231731687303715884105727)");

    $sql = 'SELECT * FROM TEST_DT';
    $query = $t->query($sql);
    while(($row = $query->fetch_array())) {
    	var_dump($row);
    }

    $query->free();
})();

?>
--EXPECTF--
array(1) {
  ["V_INT128"]=>
  string(4) "1234"
}
array(1) {
  ["V_INT128"]=>
  string(40) "-170141183460469231731687303715884105728"
}
array(1) {
  ["V_INT128"]=>
  string(39) "170141183460469231731687303715884105727"
}
