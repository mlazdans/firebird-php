--TEST--
Statement: cursor re-execute, close_cursor() and fetch state
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db("", true, false)->start_transaction();
    $t->execute("CREATE TABLE CUR (ID INTEGER)");
    $t->commit_ret();
    foreach ([1, 2, 3] as $i) $t->query("INSERT INTO CUR (ID) VALUES (?)", $i);

    $q = $t->prepare("SELECT ID FROM CUR WHERE ID >= ? ORDER BY ID");

    print "re-execute an unfinished cursor with new parameters\n";
    $q->execute(1);
    echo $q->fetch_row()[0], "\n";
    $q->execute(3);
    var_dump($q->fetch_row()[0], $q->fetch_row());

    print "execute after close_cursor()\n";
    $q->execute(2);
    echo $q->fetch_row()[0], "\n";
    $q->close_cursor();
    var_dump($q->fetch_row());
    $q->execute(2);
    echo $q->fetch_row()[0], " ", $q->fetch_row()[0], "\n";
    var_dump($q->fetch_row(), $q->fetch_row());

    print "fetch before execute\n";
    $q4 = $t->prepare("INSERT INTO CUR (ID) VALUES (?) RETURNING ID");
    foreach ([
        $t->prepare("SELECT ID FROM CUR WHERE ID = ?"),
        $t->prepare("SELECT COUNT(*) FROM CUR"),
        $q4,
    ] as $p) {
        try {
            $p->fetch_row();
            print "fetched without execute\n";
        } catch (\FireBird\Fb_Exception $e) {
            echo $e->getMessage(), "\n";
        }
    }
    $q3 = $t->prepare("SELECT COUNT(*) FROM CUR");
    $q3->execute();
    echo $q3->fetch_row()[0], "\n";

    print "RETURNING is fetched once per execute\n";
    $q4->execute(10);
    var_dump($q4->fetch_row(), $q4->fetch_row());
    $q4->execute(11);
    var_dump($q4->fetch_row()[0]);

    print "multi-row RETURNING runs at execute, not at first fetch\n";
    $t->query("UPDATE CUR SET ID = ID + 100 WHERE ID < 100 RETURNING ID");
    var_dump($t->query("SELECT MIN(ID) FROM CUR")->fetch_row()[0]);

    print "SQL ROLLBACK\n";
    $t->query("ROLLBACK");
    $t->start();
    var_dump($t->query("SELECT COUNT(*) FROM CUR")->fetch_row()[0]);
    $t->commit();
})();
?>
--EXPECT--
re-execute an unfinished cursor with new parameters
1
int(3)
NULL
execute after close_cursor()
2
NULL
2 3
NULL
NULL
fetch before execute
Statement must be executed before fetching
Statement must be executed before fetching
Statement must be executed before fetching
3
RETURNING is fetched once per execute
array(1) {
  [0]=>
  int(10)
}
NULL
int(11)
multi-row RETURNING runs at execute, not at first fetch
int(101)
SQL ROLLBACK
int(0)
