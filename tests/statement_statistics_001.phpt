--TEST--
Statement::get_statistics(): each count separately and all together
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

use FireBird\Statement;
use FireBird\Statement_Statistics;

require_once('functions.inc');

function stats(string $label, Statement $q): void
{
    $s = $q->get_statistics();
    printf("%-22s insert=%d update=%d delete=%d select=%d affected=%d\n", $label,
        $s->insert_count, $s->update_count, $s->delete_count, $s->select_count, $s->affected_count);
}

(function(){
    $t = init_tmp_db()->start_transaction();
    $t->execute("DELETE FROM TEST1");

    // insert_count
    $q = $t->query("INSERT INTO TEST1 (I, C) VALUES (1, 'a')");
    stats("insert", $q);

    $q = $t->prepare("INSERT INTO TEST1 (I, C) VALUES (?, ?)");
    $q->execute($t, 2, 'b');
    stats("insert prepared #1", $q);
    $q->execute($t, 3, 'c');
    stats("insert prepared #2", $q);

    // update_count
    $q = $t->query("UPDATE TEST1 SET C = C || '+'");
    stats("update", $q);
    $q = $t->query("UPDATE TEST1 SET C = 'x' WHERE I = -1");
    stats("update none", $q);

    // select_count
    $q = $t->query("SELECT * FROM TEST1");
    stats("select before fetch", $q);
    while ($q->fetch_row());
    stats("select after fetch", $q);

    // delete_count
    $q = $t->query("DELETE FROM TEST1 WHERE I = 3");
    stats("delete", $q);

    // Multi-row DML RETURNING is a cursor; counts are there before the first fetch
    $q = $t->query("UPDATE TEST1 SET I = I + 10 RETURNING NEW.I");
    stats("returning", $q);
    while ($q->fetch_row());
    stats("returning fetched", $q);

    // All together in one execution: 2 inserts, 4 updates (2 rows twice), 1 delete.
    // select_count includes rows located by UPDATE/DELETE: 2 + 2 + 1 + 1 + 1
    $q = $t->query(<<<SQL
        EXECUTE BLOCK AS
            DECLARE n INTEGER;
        BEGIN
            INSERT INTO TEST1 (I, C) VALUES (100, 'p');
            INSERT INTO TEST1 (I, C) VALUES (101, 'q');
            UPDATE TEST1 SET C = 'u1' WHERE I >= 100;
            UPDATE TEST1 SET C = 'u2' WHERE I >= 100;
            DELETE FROM TEST1 WHERE I = 101;
            SELECT COUNT(*) FROM TEST1 INTO n;
            SELECT I FROM TEST1 WHERE I = 100 INTO n;
        END
        SQL
    );
    stats("all together", $q);

    // Counts are per execution, not accumulated
    $q = $t->prepare("UPDATE TEST1 SET C = ? WHERE I >= ?");
    $q->execute($t, 'r1', 0);
    stats("re-execute #1", $q);
    $q->execute($t, 'r2', 100);
    stats("re-execute #2", $q);

    // Single-row RETURNING: not a cursor, output comes with execute
    $q = $t->query("INSERT INTO TEST1 (I, C) VALUES (200, 's') RETURNING I");
    stats("insert returning", $q);

    $q = $t->query("UPDATE OR INSERT INTO TEST1 (I, C) VALUES (200, 't') MATCHING (I) RETURNING I");
    stats("upsert update", $q);
    $q = $t->query("UPDATE OR INSERT INTO TEST1 (I, C) VALUES (201, 't') MATCHING (I) RETURNING I");
    stats("upsert insert", $q);

    // Multi-row MERGE RETURNING: 200 matches and is updated, 202 is inserted
    $q = $t->query(<<<SQL
        MERGE INTO TEST1 t
        USING (SELECT 200 AS I FROM RDB\$DATABASE UNION ALL SELECT 202 FROM RDB\$DATABASE) s
        ON t.I = s.I
        WHEN MATCHED THEN UPDATE SET C = 'm'
        WHEN NOT MATCHED THEN INSERT (I, C) VALUES (s.I, 'm')
        RETURNING t.I
        SQL
    );
    stats("merge returning", $q);
    while ($q->fetch_row());
    stats("merge fetched", $q);

    // Multi-row DELETE RETURNING: 200, 201, 202
    $q = $t->query("DELETE FROM TEST1 WHERE I >= 200 RETURNING I");
    stats("delete returning", $q);
    while ($q->fetch_row());
    stats("delete fetched", $q);

    var_dump((new \ReflectionClass(Statement_Statistics::class))->isFinal());

    $q->free();
    try {
        $q->get_statistics();
    } catch (\FireBird\Fb_Exception $e) {
        print get_class($e).": ".$e->getMessage()."\n";
    }

    $t->rollback();
})();

?>
--EXPECT--
insert                 insert=1 update=0 delete=0 select=0 affected=1
insert prepared #1     insert=1 update=0 delete=0 select=0 affected=1
insert prepared #2     insert=1 update=0 delete=0 select=0 affected=1
update                 insert=0 update=3 delete=0 select=3 affected=3
update none            insert=0 update=0 delete=0 select=0 affected=0
select before fetch    insert=0 update=0 delete=0 select=0 affected=0
select after fetch     insert=0 update=0 delete=0 select=3 affected=0
delete                 insert=0 update=0 delete=1 select=1 affected=1
returning              insert=0 update=2 delete=0 select=2 affected=2
returning fetched      insert=0 update=2 delete=0 select=2 affected=2
all together           insert=2 update=4 delete=1 select=7 affected=7
re-execute #1          insert=0 update=3 delete=0 select=3 affected=3
re-execute #2          insert=0 update=1 delete=0 select=1 affected=1
insert returning       insert=1 update=0 delete=0 select=0 affected=1
upsert update          insert=0 update=1 delete=0 select=1 affected=1
upsert insert          insert=1 update=0 delete=0 select=0 affected=1
merge returning        insert=1 update=1 delete=0 select=2 affected=2
merge fetched          insert=1 update=1 delete=0 select=2 affected=2
delete returning       insert=0 update=0 delete=3 select=3 affected=3
delete fetched         insert=0 update=0 delete=3 select=3 affected=3
bool(true)
FireBird\Fb_Logic_Exception: invalid statement handle
