--TEST--
Field defaults: basic fields
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');
require("common.inc");

(function(){
    $t = init_tmp_db(charset: "UTF8")->start_transaction();
    test_field_defaults($t, "FIELDS25");
})();

?>
--EXPECTF--
array(23) {
  ["ID"]=>
  int(1)
  ["SMALLINT_1"]=>
  int(-32768)
  ["INTEGER_FIELD"]=>
  int(-2147483648)
  ["BIGINT_FIELD"]=>
  int(-9223372036854775808)
  ["NUMERIC_1"]=>
  string(6) "3.1415"
  ["DECIMAL_1"]=>
  string(6) "3.1415"
  ["NUMERIC_2"]=>
  string(10) "3.14159265"
  ["DECIMAL_2"]=>
  string(10) "3.14159265"
  ["NUMERIC_3"]=>
  string(18) "3.1415926535897932"
  ["DECIMAL_3"]=>
  string(18) "3.1415926535897932"
  ["FLOAT_FIELD"]=>
  float(3.1415927410125732)
  ["DOUBLE_FIELD"]=>
  float(3.141592653589793)
  ["CHAR_FIXED"]=>
  string(40) "ABCDE                                   "
  ["VARCHAR_FIELD"]=>
  string(20) "Default varchar text"
  ["CHAR_UTF8"]=>
  string(40) "テスト                               "
  ["VARCHAR_UTF8"]=>
  string(26) "Glāžšķūņu rūķīši"
  ["DATE_FIELD"]=>
  object(DateTime) {
    ["date"]=>
    string(26) "2025-11-10 00:00:00.000000"
    ["timezone_type"]=>
    int(3)
    ["timezone"]=>
    string(3) "UTC"
  }
  ["TIME_FIELD"]=>
  object(DateTime) {
    ["date"]=>
    string(26) "%s 15:16:59.000000"
    ["timezone_type"]=>
    int(3)
    ["timezone"]=>
    string(3) "UTC"
  }
  ["TIMESTAMP_FIELD"]=>
  object(DateTime) {
    ["date"]=>
    string(26) "2025-11-10 15:16:59.000000"
    ["timezone_type"]=>
    int(3)
    ["timezone"]=>
    string(3) "UTC"
  }
  ["BINARY_FIXED"]=>
  string(64) "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
  ["VARBINARY_FIELD"]=>
  string(16) "\xDE\xAD\xBE\xEF"
  ["BLOB_TEXT"]=>
  string(20) "Hello from text blob"
  ["BLOB_BINARY"]=>
  string(22) "Hello from binary blob"
}
