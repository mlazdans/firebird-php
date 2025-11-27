--TEST--
php-firebird: ibase_trans(): handles
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $t = init_tmp_db_ibase()->new_transaction();
    $t->query("SET TRANSACTION");
    var_dump($t->id > 0);
})();

?>
--EXPECTF--
bool(true)
