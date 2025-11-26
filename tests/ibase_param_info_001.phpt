--TEST--
InterBase: ibase_param_info(): Basic test. Ported from php-firebird.
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db_ibase()->new_transaction();
    $t->start();
    $rs = $t->prepare('SELECT * FROM test1 WHERE 1 = ? AND 2 = ?');

    var_bin($rs->get_var_info_in(1));
    print "---\n";
    var_bin($rs->get_var_info_in(100));
})();

?>
--EXPECTF--
object(FireBird\Var_Info) {
  ["name"]=>
  string(0) ""
  ["alias"]=>
  string(0) ""
  ["relation"]=>
  string(0) ""
  ["byte_length"]=>
  int(4)
  ["type"]=>
  int(496)
  ["sub_type"]=>
  int(0)
  ["scale"]=>
  int(0)
  ["nullable"]=>
  bool(false)
}
---
bool(false)