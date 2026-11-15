--TEST--
Field defaults: fields introduced in FB 4.0
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');
require("common.inc");

(function(){
    $t = init_tmp_db(charset: "UTF8")->start_transaction();
    test_field_defaults($t, "FIELDS40");
})();

?>
--EXPECT--
array(8) {
  ["ID"]=>
  int(1)
  ["NUMERIC_4"]=>
  string(39) "3.1415926535897932384626433832795028841"
  ["DECIMAL_4"]=>
  string(39) "3.1415926535897932384626433832795028841"
  ["DECFLOAT_16"]=>
  string(17) "3.141592653589793"
  ["DECFLOAT_34"]=>
  string(35) "3.141592653589793238462643383279502"
  ["INT128_FIELD"]=>
  string(40) "-170141183460469231731687303715884105728"
  ["TIME_TZ"]=>
  object(DateTime) {
    ["date"]=>
    string(26) "1970-01-01 15:45:59.000000"
    ["timezone_type"]=>
    int(3)
    ["timezone"]=>
    string(13) "Europe/Berlin"
  }
  ["TIMESTAMP_TZ"]=>
  object(DateTime) {
    ["date"]=>
    string(26) "2025-11-06 15:45:59.000000"
    ["timezone_type"]=>
    int(3)
    ["timezone"]=>
    string(13) "Europe/Berlin"
  }
}
