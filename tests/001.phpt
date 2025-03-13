--TEST--
Simple operations
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php

(function(){
    require_once('functions.inc');

    if($conn = init_tmp_db()) {
        print "Connected\n";
    }
})();

?>
--EXPECT--
Connected
