--TEST--
FireBird: scrollable cursor on empty result
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);
namespace FireBirdTests;
require_once('functions.inc');
$db = init_tmp_db(); $t = $db->start_transaction();
$c = $t->open_scrollable_cursor("SELECT 1 X FROM RDB\$DATABASE WHERE 1 = 0");
var_dump($c->count(), $c->fetch_next(), $c->fetch_last(), $c->is_eof());
$c = $t->open_scrollable_cursor("SELECT 1 X FROM RDB\$DATABASE WHERE 1 = 0");
var_dump($c->fetch_last(), $c->fetch_first());
$t->rollback();

?>
--EXPECT--
int(0)
NULL
NULL
bool(true)
NULL
NULL
