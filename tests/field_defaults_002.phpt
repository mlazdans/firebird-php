--TEST--
Field defaults: fields introduced in FB 3.0
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');
require("common.inc");

(function(){
    $t = init_tmp_db(charset: "UTF8")->start_transaction();
    test_field_defaults($t, "FIELDS30");
})();

?>
--EXPECT--
array(2) {
  ["ID"]=>
  int(1)
  ["BOOL_FIELD"]=>
  bool(true)
}
