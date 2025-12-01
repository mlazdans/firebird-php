--TEST--
php-firebird: ibase_errmsg(): Basic test
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db()->start_transaction();
    try {
        $t->query('SELECT Foobar');
    } catch (\FireBird\Fb_Exception $e) {
        print $e->getMessage();
    }
})();

?>
--EXPECTF--
Dynamic SQL Error
SQL error code = -104
Unexpected end of command - line 1, column 8
