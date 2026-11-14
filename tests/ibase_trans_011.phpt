--TEST--
php-firebird: ontrol with SQL - commit explicitly
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $t = init_tmp_db()->new_transaction();
    $t->execute("SET TRANSACTION");
    $t->execute("COMMIT");
    $t->execute("COMMIT");
})();

?>
--EXPECTF--
Dynamic SQL Error
SQL error code = -901
invalid transaction handle (expecting explicit transaction start)%s
