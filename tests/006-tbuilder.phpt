--TEST--
FireBird: transaction builder
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db();

    $is_ReadConsistency_enabled = is_ReadConsistency_enabled($db);

    $test_count = 100;
    for($i = 0; $i < $test_count; $i++){
        $b = random_builder();
        $t1 = $db->new_transaction($b);
        if(!$t1->start()) {
            $b->dump_state();
            print_error_and_die("start t1", $t1);
        }
        $q = $t1->query('SELECT * FROM MON$TRANSACTIONS WHERE MON$TRANSACTION_ID = CURRENT_TRANSACTION') or print_error_and_die("query", $t1);
        $row = $q->fetch_array();

        if(false === $row) {
            print_error_and_die("fetch", $q);
        }

        unstrip_fb_specials($row);

        if(cmp_builder($t1->builder, $row, $is_ReadConsistency_enabled)){
            print "Error at test: $i\n";
            $b->dump_state();
        }

        $t1->rollback();
    }
    print "Transaction builder $test_count random builds\n";
})();

?>
--EXPECT--
Transaction builder 100 random builds