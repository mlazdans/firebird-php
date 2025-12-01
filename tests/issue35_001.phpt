--TEST--
php-firebird: Issue #35: ibase_prepare() fails to find table with SQL that has double quotes on table identifiers
--SKIPIF--
<?php
include("skipif.inc");
?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

function test35() {
    $t = init_tmp_db()->start_transaction();
    $t->query('CREATE TABLE "test" (ID INTEGER, CLIENT_NAME VARCHAR(10))');
    $t->commit_ret();

    $p = $t->prepare('INSERT INTO "test" (ID, CLIENT_NAME) VALUES (?, ?)');
    $p->execute(1, "Some name");
    $q = $t->query('SELECT * FROM "test"');
    while($r = $q->fetch_object()){
        var_dump($r);
    }
};

test35();

?>
--EXPECTF--
object(stdClass)#%d (2) {
  ["ID"]=>
  int(1)
  ["CLIENT_NAME"]=>
  string(9) "Some name"
}