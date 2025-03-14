--TEST--
Simple operations: create database, start transaction, some inserts and selects
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php

require_once('functions.inc');

(function(){
    if(false === ($conn = init_tmp_db("001.sql"))) {
        return;
    }

    $table = "TEST_001";

    print "Connected\n";

    if(false === (($t = $conn->new_transaction()) && $t->start())) {
        print_error_and_die("transaction", $conn);
    }

    $insert_tests = [
        ["INSERT INTO $table (ID, DECFLOAT_16, DECFLOAT_34) VALUES (DEFAULT, 3.141592653589793, 3.141592653589793238462643383279502) RETURNING ID", [], \FireBird\FETCH_BLOBS],
        ["INSERT INTO $table (DECFLOAT_16, DECFLOAT_34) VALUES (?, ?) RETURNING ID", ["3.141592653589793", "3.141592653589793238462643383279502"], 0],
    ];

    foreach($insert_tests as [$sql, $args, $fetch_flags]) {
        if(false === ($s = $t->query($sql, ...$args))) {
            print_error_and_die("query", $t);
        }

        while($r = $s->fetch_object($fetch_flags)) {
            print "Insert returned ";
            var_bin($r);
        }

        if(false === $r) {
            print_error_and_die("fetch", $s);
        }
    }

    $select_tests = [
        ["SELECT * FROM $table", [], \FireBird\FETCH_BLOBS],
    ];

    foreach($select_tests as [$sql, $args, $fetch_flags]) {
        if($s = $t->query($sql, ...$args)) {
            while($r = $s->fetch_object($fetch_flags)) {
                print "Row data ";
                var_bin($r);
            }

            if(false === $r) {
                print_error_and_die("fetch", $s);
            }
        } else {
            print_error_and_die("query", $t);
        }
    }

    if(!$t->commit()) {
        print_error_and_die("commit", $t);
    }
})();

?>
--EXPECT_EXTERNAL--
001.out.txt
