<?php declare(strict_types = 1);

$args = new \FireBird\Connect_Args;
$args->database = "localhost/3070:/opt/db/test.fdb";
$args->user_name = "sysdba";
$args->password = "masterkey";

$c = new \FireBird\Connector;
if (!($db = $c->connect($args))) {
    print "Connection error:\n";
    print_r($c->errors);
    exit(1);
}

print "Successfully connected!\n";

$t = $db->new_transaction();
if (!$t->start()) {
    print "Could not start transaction:\n";
    print_r($t->errors);
    exit(1);
}

if (!($q = $t->query("SELECT * FROM TEST_TABLE"))) {
    print "Could not perform a query:\n";
    print_r($t->errors);
    exit(1);
}

while ($r = $q->fetch_object(\FireBird\FETCH_BLOBS)) {
    print_r($r);
}

if (false === $r) {
    print "Fetch failed:\n";
    print_r($q->errors);
    exit(1);
}

print "Task Successfully completed!\n";
