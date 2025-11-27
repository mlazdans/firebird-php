--TEST--
php-firebird: ibase_trans(): Check order of link identifier and trans args
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db_ibase();

    $b = new \FireBird\TBuilder;
    $b->read_only(true);

    $t = $db->start_transaction($b);

    try {
        $t->query("INSERT INTO test1 VALUES(1, 2)") or die("Could not insert");
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
