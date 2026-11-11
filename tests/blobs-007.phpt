--TEST--
FireBird: BLOBs larger than one 64 KB segment
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db("", true, false)->start_transaction();
    $t->execute("CREATE TABLE B (ID INTEGER, B BLOB SUB_TYPE BINARY)");
    $t->commit_ret();

    $data = random_bytes(1024 * 1024 + 17);

    // Bound as a string
    $t->query("INSERT INTO B (ID, B) VALUES (1, ?)", $data);

    // Written in one put() and in several
    $b = $t->create_blob();
    $b->put($data);
    $b->close();
    $t->query("INSERT INTO B (ID, B) VALUES (2, ?)", $b);

    $b = $t->create_blob();
    foreach (str_split($data, 100000) as $chunk) $b->put($chunk);
    $b->close();
    $t->query("INSERT INTO B (ID, B) VALUES (3, ?)", $b->id());

    $q = $t->query("SELECT ID, B FROM B ORDER BY ID");
    while ($r = $q->fetch_row(\FireBird\FETCH_BLOB_TEXT)) {
        printf("%d: %d %s\n", $r[0], strlen($r[1]), $r[1] === $data ? "equal" : "DIFFERENT");
    }

    $id = $t->query("SELECT B FROM B WHERE ID = 1")->fetch_row()[0];
    $b = $t->open_blob($id);
    printf("total_length: %d\n", $b->total_length);
    $part = $b->get(100000);
    printf("get(100000): %d, position %d\n", strlen($part), $b->position);
    $rest = $b->get();
    printf("get(): %d, position %d, %s\n", strlen($rest), $b->position, $part . $rest === $data ? "equal" : "DIFFERENT");
    var_dump($b->get());
    $b->close();

    $t->commit();
})();
?>
--EXPECT--
1: 1048593 equal
2: 1048593 equal
3: 1048593 equal
total_length: 1048593
get(100000): 100000, position 100000
get(): 948593, position 1048593, equal
string(0) ""
