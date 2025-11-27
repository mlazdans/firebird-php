--TEST--
php-firebird: ibase_trans(): Basic operations
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db_ibase()->start_transaction();

    $sth = $t->prepare('INSERT INTO test1 VALUES (?, ?)');
    $sth->execute(100, 100);
    $rs = $t->query('SELECT * FROM test1 WHERE i = 100');
    var_dump($rs->fetch_array());
    $sth->free();
    unset($res);
})();

?>
--EXPECT--
array(2) {
  ["I"]=>
  int(100)
  ["C"]=>
  string(3) "100"
}
