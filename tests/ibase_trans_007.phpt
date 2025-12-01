--TEST--
php-firebird: ibase_trans(): handles
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $def = init_tmp_db()->start_transaction();

    $t = init_tmp_db()->new_transaction();

    $t->query("SET TRANSACTION");
    $rs = $t->query("DELETE FROM TEST1");
    var_dump($rs->affected_count);
    print "|---- TEST1 default transaction\n";
    dump_table_rows($def, "TEST1");
    print "|--------\n";
    print "|---- TEST1 t1 transaction\n";
    dump_table_rows($t, "TEST1");
    print "|--------\n";
    $t->rollback();
    $def->rollback_ret();

    print "|---- TEST1 default transaction\n";
    dump_table_rows($def, "TEST1");
    print "|--------\n";
})();

?>
--EXPECT--
int(1)
|---- TEST1 default transaction
array(2) {
  ["I"]=>
  int(1)
  ["C"]=>
  string(32) "test table not created with isql"
}
|--------
|---- TEST1 t1 transaction
|--------
|---- TEST1 default transaction
array(2) {
  ["I"]=>
  int(1)
  ["C"]=>
  string(32) "test table not created with isql"
}
|--------
