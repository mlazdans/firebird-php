--TEST--
ibase_field_info(): fields introduced in FB 3.0
--SKIPIF--
<?php
include("skipif.inc");
// skip_if_fb_lt(3);
?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');
require("common.inc");

(function(){
    $t = init_tmp_db(charset: "NONE")->start_transaction();
    test_fields30($t);
})();

?>
--EXPECT--
ID/INTEGER/4
BOOL_FIELD/BOOLEAN/1
