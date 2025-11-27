--TEST--
InterBase: ibase_affected_rows(): Basic test. Ported from php-firebird.
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db_ibase()->new_transaction();
    $t->start();
    $t->query('INSERT INTO test1 VALUES (1, 100)');
    $t->query('INSERT INTO test1 VALUES (10000, 100)');

    $rs = $t->query('UPDATE test1 SET i = 10000');
    var_dump($rs->affected_count);

    $rs = $t->query('UPDATE test1 SET i = 10000 WHERE i = 2.0');
    var_dump($rs->affected_count);

    $rs = $t->query('UPDATE test1 SET i =') or print "update\n";
    var_dump($rs->affected_count);
})();

?>
--EXPECTF--
int(3)
int(0)
Dynamic SQL Error
SQL error code = -104
Unexpected end of command - line 1, column 20%s
