--TEST--
php-firebird: ibase_trans(): handles
--SKIPIF--
<?php
include("skipif.inc");
// On FB2.5 server "invalid transaction handle" happens on fetch.
// See also: tests/ibase_trans_012.phpt
// skip_if_fb_lt(3.0);
?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $t = init_tmp_db_ibase()->start_transaction();
    $t->query("COMMIT");
    dump_rows($t->query("SELECT * FROM TEST1"));
})();

?>
--EXPECTF--
invalid transaction handle (expecting explicit transaction start)%s
