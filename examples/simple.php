<?php declare(strict_types = 1);

$args = new \FireBird\Connect_Args;
$args->database = "localhost/3070:/opt/db/test.fdb";
$args->user_name = "sysdba";
$args->password = "masterkey";

$db = (new \FireBird\Connector)->connect($args);
$t = $db->new_transaction();
$t->start();
$q = $t->query("SELECT * FROM TEST_TABLE");
while ($r = $q->fetch_object(\FireBird\FETCH_BLOBS)) {
    print_r($r);
}
