--TEST--
InterBase: ibase_num_params(): Basic test. Ported from php-firebird.
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db_ibase()->new_transaction();
    $t->start() or print_error_and_die("tr start", $t);
    $rs = $t->prepare('SELECT * FROM test1 WHERE 1 = ? AND 2 = ?');
    var_dump($rs->num_vars_in);
})();

?>
--EXPECTF--
int(2)
