--TEST--
FireBird: BLOBs cancel missuse
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db();
    $t = $db->start_transaction();
    $b = $t->create_blob();
    $b->put("Blob created with \FireBird\Transaction");
    $b->cancel(); // cancelled

    $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", $b);
})();

?>
--EXPECTF--
invalid BLOB ID%s
