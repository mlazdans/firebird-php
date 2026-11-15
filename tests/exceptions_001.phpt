--TEST--
Exceptions: Fb_Server_Exception / Fb_Value_Exception / Fb_Logic_Exception, status items
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

function show(\Throwable $e): void
{
    printf("%s (Fb_Exception: %s, file_ext set: %s)\n", get_class($e),
        var_export($e instanceof \FireBird\Fb_Exception, true),
        var_export($e instanceof \FireBird\Fb_Exception && $e->file_ext !== "" && $e->line_ext > 0, true));
}

(function(){
    $t = init_tmp_db()->start_transaction();
    $t->execute("CREATE TABLE T (ID INTEGER NOT NULL CONSTRAINT PK_T PRIMARY KEY, S SMALLINT)");
    $t->commit_ret();
    $t->execute("INSERT INTO T (ID) VALUES (1)");

    print "--- server: unique key\n";
    try {
        $t->execute("INSERT INTO T (ID) VALUES (1)");
    } catch (\FireBird\Fb_Server_Exception $e) {
        show($e);
        var_dump($e->sqlstate, $e->getCode(), $e->getCode() === $e->status[0]->code, $e->warnings);
        foreach ($e->status as $item) {
            printf("%s %d %s\n", get_class($item), $item->code, $item->message);
        }
        var_dump($e->getMessage() === implode("\n", array_map(fn($i) => $i->message, $e->status)));
    }

    print "--- value\n";
    try {
        $t->query("INSERT INTO T (ID, S) VALUES (?, ?)", 2, 32768);
    } catch (\FireBird\Fb_Value_Exception $e) {
        show($e);
        print $e->getMessage()."\n";
    }

    print "--- value: method argument\n";
    try {
        (new \FireBird\Transaction_Builder)->wait(40000);
    } catch (\FireBird\Fb_Value_Exception $e) {
        show($e);
    }

    print "--- usage\n";
    try {
        $t->query("SELECT * FROM T WHERE ID = ?");
    } catch (\FireBird\Fb_Logic_Exception $e) {
        show($e);
        print $e->getMessage()."\n";
    }

    print "--- usage, reported with a Firebird code\n";
    try {
        $t->prepare_scrollable_cursor("SELECT * FROM T")->fetch_next();
    } catch (\FireBird\Fb_Logic_Exception $e) {
        show($e);
        var_dump($e->getCode(), $e->getMessage());
    }

    print "--- PHP argument types stay built-in\n";
    try {
        (new \FireBird\Transaction_Builder)->wait("x");
    } catch (\TypeError $e) {
        show($e);
    }
})();

?>
--EXPECT--
--- server: unique key
FireBird\Fb_Server_Exception (Fb_Exception: true, file_ext set: true)
string(5) "23000"
int(335544665)
bool(true)
array(0) {
}
FireBird\Fb_Status_Item 335544665 violation of PRIMARY or UNIQUE KEY constraint "PK_T" on table "T"
FireBird\Fb_Status_Item 335545072 Problematic key value is ("ID" = 1)
bool(true)
--- value
FireBird\Fb_Value_Exception (Fb_Exception: true, file_ext set: true)
Argument 2: value 32768 is out of range for SMALLINT
--- value: method argument
FireBird\Fb_Value_Exception (Fb_Exception: true, file_ext set: true)
--- usage
FireBird\Fb_Logic_Exception (Fb_Exception: true, file_ext set: true)
Statement expects 1 arguments, 0 given
--- usage, reported with a Firebird code
FireBird\Fb_Logic_Exception (Fb_Exception: true, file_ext set: true)
int(336003095)
string(18) "Cursor is not open"
--- PHP argument types stay built-in
TypeError (Fb_Exception: false, file_ext set: false)
