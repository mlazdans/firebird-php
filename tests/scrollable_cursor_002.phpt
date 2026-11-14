--TEST--
FireBird: scrollable cursor count() before first fetch
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);
namespace FireBirdTests;
require_once('functions.inc');
$db = init_tmp_db();
$t = $db->start_transaction();
$t->execute("CREATE TABLE S (ID INT)");
$t->commit_ret();
for ($i = 1; $i <= 5; $i++) $t->query("INSERT INTO S VALUES (?)", $i);
$sqls = ["SELECT ID FROM S ORDER BY ID", "SELECT ID FROM S WHERE ID <= ? ORDER BY ID"];
foreach ($sqls as $sql) {
  foreach ([["count","last"],["count","next"],["count","first"],["count","absolute"],["last"],["next","count","prior"]] as $seq) {
    $c = $t->open_scrollable_cursor($sql, ...(str_contains($sql, "?") ? [3] : []));
    $out = [];
    foreach ($seq as $op) {
        try {
            $r = match($op) { "count" => $c->count(), "absolute" => $c->fetch_absolute(2), default => $c->{"fetch_$op"}() };
            $out[] = "$op=" . (is_object($r) ? $r->ID : var_export($r, true));
        } catch (\Throwable $e) { $out[] = "$op=ERR(" . strtok($e->getMessage(), "\n") . "/" . $e->getCode() . ")"; }
    }
    echo (str_contains($sql, "?") ? "param  " : "noparam"), " ", implode(" ", $out), "\n";
  }
}
$t->rollback();

?>
--EXPECT--
noparam count=5 last=5
noparam count=5 next=1
noparam count=5 first=1
noparam count=5 absolute=2
noparam last=5
noparam next=1 count=5 prior=NULL
param   count=3 last=3
param   count=3 next=1
param   count=3 first=1
param   count=3 absolute=2
param   last=3
param   next=1 count=3 prior=NULL
