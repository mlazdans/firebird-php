--TEST--
php-firebird: Long names: Firebird 3.0 or older
--SKIPIF--
<?php
require_once("skipif.inc");
// TODO: service
// skip_if_fb_gt(3.0);
?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');
require_once('common.inc');

// FB 2.5, 3.0 identifier len is by byte count not character count
$MAX_LEN = 31;
test_max_len(31, intdiv(31, 4));

?>
--EXPECT--
int(31)
Table:TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
array(2) {
  ["FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"]=>
  int(1)
  ["🥰🥰🥰🥰🥰🥰🥰"]=>
  int(2)
}
Table:😂😂😂😂😂😂😂
array(2) {
  ["FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"]=>
  int(1)
  ["🥰🥰🥰🥰🥰🥰🥰"]=>
  int(2)
}