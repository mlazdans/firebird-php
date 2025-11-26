--TEST--
InterBase: ibase_rollback(): Basic test. Ported from php-firebird.
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db_ibase()->new_transaction();
    $t->start();

    $queries = [
        'INSERT INTO test1 VALUES (100, 2)',
        'INSERT INTO test1 VALUES (100, 2)',
        'INSERT INTO test1 VALUES (100, 2)',
    ];

    query_bulk($t, $queries);

    $rs = $t->query('SELECT COUNT(*) FROM test1 WHERE i = 100');
    var_dump($rs->fetch_row());

    $t->rollback_ret();
    print "rollback_ret OK\n";

    $rs = $t->query('SELECT COUNT(*) FROM test1 WHERE i = 100');
    var_dump($rs->fetch_row());

    $t->rollback();
    print "rollback 1 OK\n";

    $t->rollback();
    print "rollback 2 OK\n";
})();

?>
--EXPECTF--
array(1) {
  [0]=>
  int(3)
}
rollback_ret OK
array(1) {
  [0]=>
  int(0)
}
rollback 1 OK
Invalid transaction pointer
