--TEST--
ibase_field_info(): fields introduced in FB 4.0
--SKIPIF--
<?php
include("skipif.inc");
// skip_if_fb_lt(4);
// skip_if_fbclient_lt(4);
?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');
require("common.inc");

(function(){
    $t = init_tmp_db(charset: "NONE")->start_transaction();
    test_fields40($t);
})();

?>
--EXPECT--
ID/INTEGER/4
NUMERIC_4/NUMERIC(38,37)/16
DECIMAL_4/NUMERIC(38,37)/16
DECFLOAT_16/DECFLOAT(16)/8
DECFLOAT_34/DECFLOAT(34)/16
INT128_FIELD/INT128/16
TIME_TZ/TIME WITH TIME ZONE/8
TIMESTAMP_TZ/TIMESTAMP WITH TIME ZONE/12