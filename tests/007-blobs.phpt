--TEST--
FireBird: BLOBs
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    if(false === ($db = init_tmp_db())) {
        return;
    }

    if(false === ($t = $db->new_transaction())) {
        print_error_and_die("transaction", $db);
    }

    $t->start() or print_error_and_die("transaction start", $t);
    $t->query(load_file_or_die(Config::$pwd."/001-table.sql")) or print_error_and_die("create_table", $t);
    $t->commit_ret() or print_error_and_die("commit_ret", $t);

    $table = "TEST_001";
    $queries = [
        ["INSERT INTO $table (BLOB_1) VALUES ('Inlined argument')", []],
        ["INSERT INTO $table (BLOB_1) VALUES (?)", ["Binded argument"]],
    ];

    $data = "Blob created with \FireBird\Transaction";
    $b = $t->create_blob() or print_error_and_die("\$t->create_blob", $t);
    $b->put($data) or print_error_and_die("blob put", $b);
    print "[1] ";
    printf("Expected is_writable=1, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", strlen($data), $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", strlen($data), $b->total_length);
    $b->close() or print_error_and_die("blob close", $b);
    $queries[] = ["INSERT INTO $table (BLOB_1) VALUES (?)", [$b]];
    print "\n";

    $data = "Blob created with \FireBird\Blob";
    $b = new \FireBird\Blob($t);
    $b = $t->create_blob() or print_error_and_die("\$t->create_blob", $t);
    $b->put($data) or print_error_and_die("blob put", $b);
    print "[2] ";
    printf("Expected is_writable=1, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", strlen($data), $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", strlen($data), $b->total_length);
    $b->close() or print_error_and_die("blob close", $b);
    $queries[] = ["INSERT INTO $table (BLOB_1) VALUES (?)", [$b]];
    print "\n";

    $l = 0;
    $b = $t->create_blob() or print_error_and_die("\$t->create_blob", $t);
    for($i = 0; $i < 10; $i++) {
        $data = "Put number: $i\n";
        $b->put($data) or print_error_and_die("blob put", $b);
        $l += strlen($data);
    }
    print "[3] ";
    printf("Expected is_writable=1, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", $l, $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", $l, $b->total_length);
    $b->close() or print_error_and_die("blob close", $b);
    $queries[] = ["INSERT INTO $table (BLOB_1) VALUES (?)", [$b]];
    print "\n";

    $b = $t->create_blob() or print_error_and_die("\$t->create_blob", $t);
    $b->put("Will cancel this one") or print_error_and_die("blob put", $b);
    $b->cancel() or print_error_and_die("blob cancel", $b);

    query_bulk_or_die($t, $queries);

    $q = query_or_die($t, "SELECT BLOB_1 FROM $table");
    fetch_and_print_or_die($q, \FireBird\FETCH_BLOBS);

    $t->commit() or print_error_and_die("commit", $t);

    // Fetching
    $t->start() or print_error_and_die("start", $t);
    $q = query_or_die($t, "SELECT ID, BLOB_1 FROM $table ORDER BY ID");

    $data = 'Inlined argument';
    $r = $q->fetch_object() or print_error_and_die("was expecing a row", $q);
    print "[4] ";
    printf("BLOB_1 instanceof \FireBird\Blob_Id: %d\n", $r->BLOB_1 instanceof \FireBird\Blob_Id);
    $b = $t->open_blob($r->BLOB_1) or print_error_and_die("open blob", $t);
    printf("Expected is_writable=0, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", 0, $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", strlen($data), $b->total_length);
    print "ID: $r->ID, BLOB_1 = '".$b->get()."'\n";
    printf("Expected b->position=%d, actual=%d\n", strlen($data), $b->position);
    $b->close() or print_error_and_die("blob close", $b);
    print "\n";

    $data = 'Binded argument';
    $max_len = 5;
    $r = $q->fetch_object() or print_error_and_die("was expecing a row", $q);
    print "[5] ";
    printf("BLOB_1 instanceof \FireBird\Blob_Id: %d\n", $r->BLOB_1 instanceof \FireBird\Blob_Id);
    $b = $t->open_blob($r->BLOB_1) or print_error_and_die("open blob", $t);
    printf("Expected is_writable=0, actual=%d\n", $b->is_writable);
    printf("Expected b->position=%d, actual=%d\n", 0, $b->position);
    printf("Expected b->total_length=%d, actual=%d\n", strlen($data), $b->total_length);
    print "ID: $r->ID, BLOB_1 ($max_len bytes) = '".$b->get($max_len)."'\n";
    printf("Expected b->position=%d, actual=%d\n", $max_len, $b->position);
    $b->close() or print_error_and_die("blob close", $b);

    $t->commit() or print_error_and_die("commit", $t);

    // Error handling
    $t->start() or print_error_and_die("start", $t);
    try {
        $b->open($GLOBALS['945903845093458768'] ?? null);
    } catch (\TypeError $e) {
        print $e->getMessage()."\n";
    }

    $q = query_or_die($t, "SELECT ID, BLOB_1 FROM $table ORDER BY ID");
    $r = $q->fetch_object() or print_error_and_die("was expecing a row", $q);
    $b = $t->open_blob($r->BLOB_1) or print_error_and_die("open blob", $t);
    $b->put("This should not be possible") or print "$b->error_msg\n";
    $b->close() or print_error_and_die("blob close", $b);
    $b->close() or print "$b->error_msg\n";
})();

?>
--EXPECT_EXTERNAL--
007-blobs.out.txt
