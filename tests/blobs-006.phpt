--TEST--
FireBird: BLOB IDs
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
    $b->close();

    $id_str = (string)$b->id();
    $id2 = \FireBird\Blob_Id::from_str($id_str);

    var_dump($id_str === (string)$id2);

    $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", $id2);
    dump_rows($t->query("SELECT BLOB_1 FROM TEST_001"), \FireBird\FETCH_BLOB_TEXT);
})();

?>
--EXPECT--
bool(true)
array(1) {
  ["BLOB_1"]=>
  string(39) "Blob created with \FireBird\Transaction"
}
