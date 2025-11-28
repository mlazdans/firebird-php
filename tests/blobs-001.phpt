--TEST--
FireBird: BLOBs
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db();
    $t = $db->start_transaction();

    $t->execute(load_file_or_die(Config::$pwd."/001-table.sql"));
    $t->commit_ret();

    $table = "TEST_001";
    $queries = [
        ["INSERT INTO $table (BLOB_1) VALUES ('Inlined argument')", []],
        ["INSERT INTO $table (BLOB_1) VALUES (?)", ["Binded argument"]],
    ];

    $data = "Blob created with \FireBird\Transaction";
    $b = $t->create_blob();
    $b->put($data);
    print "[1] ";
    printf("Expected is_writable=1, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", strlen($data), $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", strlen($data), $b->total_length);
    $b->close();
    $queries[] = ["INSERT INTO $table (BLOB_1) VALUES (?)", [$b]];
    print "\n";

    $data = "Blob created with \FireBird\Blob";
    $b = new \FireBird\Blob($t);
    $b = $t->create_blob();
    $b->put($data);
    print "[2] ";
    printf("Expected is_writable=1, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", strlen($data), $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", strlen($data), $b->total_length);
    $b->close();
    $queries[] = ["INSERT INTO $table (BLOB_1) VALUES (?)", [$b]];
    print "\n";

    $l = 0;
    $b = $t->create_blob();
    for($i = 0; $i < 10; $i++) {
        $data = "Put number: $i\n";
        $b->put($data);
        $l += strlen($data);
    }
    print "[3] ";
    printf("Expected is_writable=1, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", $l, $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", $l, $b->total_length);
    $b->close();
    $queries[] = ["INSERT INTO $table (BLOB_1) VALUES (?)", [$b]];
    $queries[] = ["INSERT INTO $table (BLOB_1) VALUES (?)", [null]];
    $queries[] = ["INSERT INTO $table (BLOB_1) VALUES (?)", ["Some textual argument"]];
    $queries[] = "INSERT INTO $table (BLOB_1) VALUES ('Some inlined argument')";
    print "\n";

    $b = $t->create_blob();
    $b->put("Will cancel this one");
    $b->cancel();

    query_bulk($t, $queries);

    $q = $t->query("SELECT BLOB_1 FROM $table");
    fetch_and_print($q, \FireBird\FETCH_FETCH_BLOB_TEXT);

    $t->commit();

    // Fetching
    $t->start();
    $q = $t->query("SELECT ID, BLOB_1 FROM $table ORDER BY ID");

    $data = 'Inlined argument';
    $r = $q->fetch_object();
    print "[4] ";
    printf("BLOB_1 instanceof \FireBird\Blob_Id: %d\n", $r->BLOB_1 instanceof \FireBird\Blob_Id);
    $b = $t->open_blob($r->BLOB_1);
    printf("Expected is_writable=0, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", 0, $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", strlen($data), $b->total_length);
    print "ID: $r->ID, BLOB_1 = '".$b->get()."'\n";
    printf("Expected b->position=%d, actual=%d\n", strlen($data), $b->position);
    $b->close();
    print "\n";

    $data = 'Binded argument';
    $max_len = 5;
    $r = $q->fetch_object();
    print "[5] ";
    printf("BLOB_1 instanceof \FireBird\Blob_Id: %d\n", $r->BLOB_1 instanceof \FireBird\Blob_Id);
    $b = new \FireBird\Blob($t);
    $b->open($r->BLOB_1);
    printf("Expected is_writable=0, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", 0, $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", strlen($data), $b->total_length);
    print "ID: $r->ID, BLOB_1 ($max_len bytes) = '".$b->get($max_len)."'\n";
    printf("Expected b->position=%d, actual=%d\n", $max_len, $b->position);
    $b->close();

    $t->commit();

    // Error handling
    $t->start();
    try {
        $b->open($GLOBALS['945903845093458768'] ?? null);
    } catch (\TypeError $e) {
        print $e->getMessage()."\n";
    }

    $q = $t->query("SELECT ID, BLOB_1 FROM $table ORDER BY ID");
    $r = $q->fetch_object();
    $b = $t->open_blob($r->BLOB_1);

    try {
        $b->put("This should not be possible");
        print "possible\n";
    } catch (\Exception $e) {
        print $e->getMessage()."\n";
    }
    $b->close();
    $b->close();

    $q = $t->query("SELECT ID, BLOB_1 FROM $table WHERE ID = 5");

    $r = $q->fetch_object();
    $b = $t->open_blob($r->BLOB_1);

    $b->seek(4 * (strlen("Put number: 0") + 1), \FireBird\BLOB_SEEK_START);
    $data = $b->get(strlen("Put number: 0") + 1);
    print "Seeked data: $data";

    $b->seek(-2 * (strlen("Put number: 0") + 1), \FireBird\BLOB_SEEK_END);
    $data = $b->get(strlen("Put number: 0") + 1);
    print "Seeked data: $data";
    $b->close();
})();

?>
--EXPECT_EXTERNAL--
blobs-001.out.txt
