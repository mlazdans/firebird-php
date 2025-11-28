--TEST--
InterBase: ibase_num_params(): Basic test. Ported from php-firebird.
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db()->start_transaction();
    $rs = $t->prepare('SELECT * FROM test1 WHERE 1 = ? AND 2 = ?');
    var_dump($rs->in_vars_count);
})();

?>
--EXPECTF--
int(2)
