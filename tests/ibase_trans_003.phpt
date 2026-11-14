--TEST--
php-firebird: ibase_trans(): Check order of link identifier and trans args
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db();

    $b = new \FireBird\Transaction_Builder;
    $b->read_only(true);

    $t = $db->start_transaction($b);

    try {
        $t->execute("INSERT INTO test1 VALUES(1, 2)");
        die("Could insert");
    } catch (\FireBird\Fb_Exception $e) {
        die("Could not insert");
    }

    $t->commit();
    print "Finished OK\n";

    unset($db);
})();


?>
--EXPECT--
Could not insert
