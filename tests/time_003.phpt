--TEST--
php-firebird: IBASE_UNIXTIME: ignore IBASE_UNIXTIME flag for TIME fields
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

function test_time_unixtime(): void
{
    // ini_set("ibase.timeformat", "%H:%M:%S");
    $t = init_tmp_db_ibase()->start_transaction();

    $t->query("CREATE TABLE TTEST (
        ID INTEGER,
        T1 TIME DEFAULT '15:45:59',
        T2 TIMESTAMP DEFAULT '2025-11-06 15:45:59'
    )");
    $t->commit_ret();

    $t->query("INSERT INTO TTEST (ID) VALUES (1)");
    dump_table_rows($t, "TTEST");
    dump_table_rows($t, "TTEST", \FireBird\FETCH_UNIXTIME);
}

test_time_unixtime();

?>
--EXPECTF--
array(3) {
  ["ID"]=>
  int(1)
  ["T1"]=>
  string(8) "15:45:59"
  ["T2"]=>
  string(19) "2025-11-06 15:45:59"
}
array(3) {
  ["ID"]=>
  int(1)
  ["T1"]=>
  string(8) "15:45:59"
  ["T2"]=>
  int(1762443959)
}