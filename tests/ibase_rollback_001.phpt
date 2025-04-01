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
    $t->start() or print_error_and_die("tr start", $t);

    $queries = [
        'INSERT INTO test1 VALUES (100, 2)',
        'INSERT INTO test1 VALUES (100, 2)',
        'INSERT INTO test1 VALUES (100, 2)',
    ];

    query_bulk_or_die($t, $queries);

    $rs = $t->query('SELECT COUNT(*) FROM test1 WHERE i = 100') or print_error_and_die("query1", $t);
    var_dump($rs->fetch_row());

    var_dump($t->rollback_ret());

    $rs = $t->query('SELECT COUNT(*) FROM test1 WHERE i = 100') or print_error_and_die("query2", $t);
    var_dump($rs->fetch_row());

    $r = $t->rollback() or print "$t->error_msg\n";
    var_dump($r);

    $r = $t->rollback() or print "$t->error_msg\n";
    var_dump($r);
})();

?>
--EXPECTF--
array(1) {
  [0]=>
  int(3)
}
bool(true)
array(1) {
  [0]=>
  int(0)
}
bool(true)
invalid transaction handle (expecting explicit transaction start)
bool(false)
