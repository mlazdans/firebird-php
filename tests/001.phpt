--TEST--
Simple operations
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php

(function(){
    require_once('config.inc');
    require_once('functions.inc');
    $db = new \FireBird\Database(
        "$host:/opt/db/test.fdb",
        username: $user,
        password: $password,
    );

    $conn = new \FireBird\Connection($db);
    if($conn->connect()) {
        print "Connected\n";
    } else {
        print_error($conn);
    }
})();

?>
--EXPECT--
Connected
