--TEST--
InterBase: ibase_param_info(): Basic test. Ported from php-firebird.
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db()->start_transaction();
    $rs = $t->prepare('SELECT * FROM test1 WHERE 1 = ? AND 2 = ?');

    var_bin($rs->get_var_info_in(0));
    print "---\n";
    var_bin($rs->get_var_info_in(100));
})();

?>
--EXPECTF--
object(FireBird\Var_Info) {
  ["field"]=>
  string(0) ""
  ["alias"]=>
  string(0) ""
  ["relation"]=>
  string(0) ""
  ["type_str"]=>
  string(7) "INTEGER"
  ["type"]=>
  int(496)
  ["subtype"]=>
  int(0)
  ["length"]=>
  int(4)
  ["scale"]=>
  int(0)
  ["is_nullable"]=>
  bool(false)
}
---
Invalid var index%s
