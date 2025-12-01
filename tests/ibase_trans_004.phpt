--TEST--
php-firebird: ibase_trans(): handles
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $t = init_tmp_db()->start_transaction();
    $t->query("COMMIT");
    dump_rows($t->query("SELECT * FROM TEST1"));
})();

?>
--EXPECTF--
invalid transaction handle (expecting explicit transaction start)%s
