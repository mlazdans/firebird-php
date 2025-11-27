--TEST--
IBASE_UNIXTIME: ignore IBASE_UNIXTIME flag for TIME_TZ fields
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

function test_time_tz_unixtime(): void
{
    $t = init_tmp_db_ibase()->start_transaction();
    $t->query("CREATE TABLE TTEST (
        ID INTEGER,
        T1 TIME WITH TIME ZONE DEFAULT '15:45:59 Europe/Riga',
        T2 TIMESTAMP WITH TIME ZONE DEFAULT '2025-11-06 15:45:59 Europe/Riga'
    )");
    $t->commit_ret();
    $t->query("INSERT INTO TTEST (ID) VALUES (1)");
    dump_table_rows($t, "TTEST");
    dump_table_rows($t, "TTEST", \FireBird\FETCH_UNIXTIME);
}

test_time_tz_unixtime();

?>
--EXPECTF--
array(3) {
  ["ID"]=>
  int(1)
  ["T1"]=>
  string(20) "15:45:59 Europe/Riga"
  ["T2"]=>
  string(31) "2025-11-06 15:45:59 Europe/Riga"
}
array(3) {
  ["ID"]=>
  int(1)
  ["T1"]=>
  string(20) "15:45:59 Europe/Riga"
  ["T2"]=>
  int(1762443959)
}