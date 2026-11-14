--TEST--
FireBird: scrollable cursor
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);
namespace FireBirdTests;
require_once('functions.inc');
use FireBird\Scrollable_Cursor;

function v(?object $o) { return $o === null ? "null" : json_encode($o); }
function t(string $label, callable $f) {
    try { $r = $f(); }
    catch (\Throwable $e) { $r = get_class($e) . ": " . $e->getMessage(); }
    echo "$label: $r\n";
}

$db = init_tmp_db();
$t = $db->start_transaction();
$t->execute("CREATE TABLE S (ID INT, NAME VARCHAR(10), B BLOB SUB_TYPE TEXT)");
$t->execute("CREATE PROCEDURE P(N INT) RETURNS (I INT) AS BEGIN I = 1; WHILE (I <= N) DO BEGIN SUSPEND; I = I + 1; END END");
$t->commit_ret();
for ($i = 1; $i <= 5; $i++) $t->query("INSERT INTO S VALUES (?, ?, ?)", $i, "n$i", "blob$i");

$c = $t->open_scrollable_cursor("SELECT ID, NAME FROM S WHERE ID <= ? ORDER BY ID", 5);
t("class", fn() => get_class($c) . " db=" . ($c->database === $db ? "same" : "diff"));
t("bof before", fn() => var_export($c->is_bof(), true));
t("next", fn() => v($c->fetch_next()));
t("next", fn() => v($c->fetch_next()));
t("prior", fn() => v($c->fetch_prior()));
t("prior", fn() => v($c->fetch_prior()));
t("bof", fn() => var_export($c->is_bof(), true));
t("last", fn() => v($c->fetch_last()));
t("next past end", fn() => v($c->fetch_next()));
t("eof", fn() => var_export($c->is_eof(), true));
t("prior after end", fn() => v($c->fetch_prior()));
t("first", fn() => v($c->fetch_first()));
t("absolute 3", fn() => v($c->fetch_absolute(3)));
t("absolute -2", fn() => v($c->fetch_absolute(-2)));
t("absolute 0", fn() => v($c->fetch_absolute(0)));
t("absolute 99", fn() => v($c->fetch_absolute(99)));
t("relative -2", fn() => v($c->fetch_relative(-2)));
t("relative 1", fn() => v($c->fetch_relative(1)));
t("count", fn() => $c->count());
t("next after count", fn() => v($c->fetch_next()));
t("close", function() use ($c) { $c->close(); return "ok"; });
t("fetch after close", fn() => v($c->fetch_next()));
t("close again", function() use ($c) { $c->close(); return "ok"; });
t("abs out of int", fn() => v((function() use ($t) { $x = $t->open_scrollable_cursor("SELECT 1 FROM RDB\$DATABASE"); return $x->fetch_absolute(PHP_INT_MAX); })()));

echo "-- prepared, reexecute\n";
$p = $t->prepare_scrollable_cursor("SELECT ID FROM S WHERE ID <= ? ORDER BY ID");
t("fetch before execute", fn() => v($p->fetch_next()));
t("bof before execute", fn() => var_export($p->is_bof(), true));
$p->execute($t, 2);
t("count 2", fn() => $p->count());
t("last", fn() => v($p->fetch_last()));
$p->execute($t, 4);
t("count 4", fn() => $p->count());
t("last", fn() => v($p->fetch_last()));
$p2 = $db->prepare_scrollable_cursor($t, "SELECT ID FROM S ORDER BY ID DESC");
$p2->execute($t);
t("db prepare first", fn() => v($p2->fetch_first()));

echo "-- statement kinds\n";
t("selectable proc", fn() => v($t->open_scrollable_cursor("SELECT I FROM P(4)")->fetch_last()));
t("function", fn() => v($t->open_scrollable_cursor("SELECT UPPER(NAME) U FROM S ORDER BY ID")->fetch_absolute(2)));
t("exec block", fn() => v($t->open_scrollable_cursor("EXECUTE BLOCK RETURNS (X INT) AS BEGIN X = 7; SUSPEND; X = 8; SUSPEND; END")->fetch_last()));
t("update returning", fn() => v((function() use ($t) { $x = $t->open_scrollable_cursor("UPDATE S SET NAME = NAME || 'x' WHERE ID > 3 RETURNING ID, NAME"); $x->fetch_last(); return $x->fetch_prior(); })()));
t("insert values returning", fn() => v($t->open_scrollable_cursor("INSERT INTO S (ID) VALUES (9) RETURNING ID")->fetch_next()));
t("execute procedure", fn() => v($t->open_scrollable_cursor("EXECUTE PROCEDURE P(1)")->fetch_next()));
t("plain insert", fn() => v($t->open_scrollable_cursor("INSERT INTO S (ID) VALUES (10)")->fetch_next()));
t("blob text", fn() => v($t->open_scrollable_cursor("SELECT B FROM S ORDER BY ID")->fetch_last(\FireBird\FETCH_BLOB_TEXT)));
t("blob after prior", fn() => v((function() use ($t) { $x = $t->open_scrollable_cursor("SELECT B FROM S ORDER BY ID"); $x->fetch_last(); return $x->fetch_prior(\FireBird\FETCH_BLOB_TEXT); })()));
t("wrong db tr", fn() => v((function() use ($p) { $t2 = init_tmp_db()->start_transaction(); $p->execute($t2, 1); return null; })()));
t("new direct", fn() => new Scrollable_Cursor);
$t->rollback();
t("fetch after tr gone", fn() => v($c->fetch_next()));
echo "done\n";

?>
--EXPECT--
class: FireBird\Scrollable_Cursor db=same
bof before: true
next: {"ID":1,"NAME":"n1"}
next: {"ID":2,"NAME":"n2"}
prior: {"ID":1,"NAME":"n1"}
prior: null
bof: true
last: {"ID":5,"NAME":"n5"}
next past end: null
eof: true
prior after end: {"ID":5,"NAME":"n5"}
first: {"ID":1,"NAME":"n1"}
absolute 3: {"ID":3,"NAME":"n3"}
absolute -2: {"ID":4,"NAME":"n4"}
absolute 0: null
absolute 99: null
relative -2: {"ID":4,"NAME":"n4"}
relative 1: {"ID":5,"NAME":"n5"}
count: 5
next after count: null
close: ok
fetch after close: FireBird\Fb_Exception: Cursor is not open
close again: FireBird\Fb_Exception: Invalid cursor pointer
abs out of int: ValueError: FireBird\Scrollable_Cursor::fetch_absolute(): Argument #1 ($position) must be between -2147483648 and 2147483647
-- prepared, reexecute
fetch before execute: FireBird\Fb_Exception: Cursor is not open
bof before execute: FireBird\Fb_Exception: Cursor is not open
count 2: 2
last: {"ID":2}
count 4: 4
last: {"ID":4}
db prepare first: {"ID":5}
-- statement kinds
selectable proc: {"I":4}
function: {"U":"N2"}
exec block: {"X":8}
update returning: {"ID":4,"NAME":"n4x"}
insert values returning: FireBird\Fb_Exception: Cannot open cursor for non-SELECT statement
execute procedure: FireBird\Fb_Exception: Cannot open cursor for non-SELECT statement
plain insert: FireBird\Fb_Exception: Cannot open cursor for non-SELECT statement
blob text: {"B":"blob5"}
blob after prior: {"B":"blob4"}
wrong db tr: ValueError: Transaction belongs to a different database than the statement
new direct: Error: Call to private FireBird\Scrollable_Cursor::__construct() from global scope
fetch after tr gone: FireBird\Fb_Exception: Cursor is not open
done
