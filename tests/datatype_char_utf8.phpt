--TEST--
Test CHAR fields and UTF8
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db()->start_transaction();

    $t->query(
        "CREATE TABLE test_dt (
            V_CHAR_UTF8_1 CHAR(1) CHARACTER SET UTF8,
            V_CHAR_UTF8_2 CHAR(22) CHARACTER SET UTF8,
            V_CHAR_UTF8_3 CHAR(7) CHARACTER SET UTF8,
            V_VARCHAR_UTF8_1 VARCHAR(1) CHARACTER SET UTF8
        )");
    $t->commit_ret();

    $t->query(
        "INSERT INTO TEST_DT (
            V_CHAR_UTF8_1,
            V_CHAR_UTF8_2,
            V_CHAR_UTF8_3,
            V_VARCHAR_UTF8_1
        ) VALUES (
            '€',
            ' A €  A  €   A   €    ',
            ' A A A',
            '€ '
        )");

    $query = $t->query('SELECT * FROM TEST_DT');
    while(($row = $query->fetch_array())) {
    	var_dump($row);
    }

    $query->free();
})();

?>
--EXPECTF--
array(4) {
  ["V_CHAR_UTF8_1"]=>
  string(4) "€ "
  ["V_CHAR_UTF8_2"]=>
  string(88) " A €  A  €   A   €                                                                "
  ["V_CHAR_UTF8_3"]=>
  string(28) " A A A                      "
  ["V_VARCHAR_UTF8_1"]=>
  string(3) "€"
}