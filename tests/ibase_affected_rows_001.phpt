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
    $t->start() or print_error_and_die("tr start", $t);
    $t->query('INSERT INTO test1 VALUES (1, 100)');
    $t->query('INSERT INTO test1 VALUES (10000, 100)');

    $rs = $t->query('UPDATE test1 SET i = 10000') or print_error_and_die("update1", $t);
    var_dump($rs->affected_count);

    $rs = $t->query('UPDATE test1 SET i = 10000 WHERE i = 2.0') or print_error_and_die("update2", $t);
    var_dump($rs->affected_count);

    $rs = $t->query('UPDATE test1 SET i =') or print "$t->error_msg\n";
    var_dump($rs->affected_count);
})();

?>
--EXPECTF--
int(3)
int(0)
Dynamic SQL Error
SQL error code = -104
Unexpected end of command - line 1, column 20

Warning: Attempt to read property "affected_count" on false in %s on line %d
NULL