--TEST--
Properties of statements, transactions and blobs after their database is gone
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $args = new \FireBird\Create_Args;
    $args->database = gen_random_db_file_name_or_die();
    args_apply_defaults($args);
    $db = \FireBird\Database::create($args);

    $t = $db->start_transaction();
    $q = $t->query("SELECT 1 FROM RDB\$DATABASE");
    $b = $t->create_blob();

    var_dump($q->name, $q->out_vars_count, $t->id > 0, $b->is_writable);

    $t->commit();
    $db->drop();

    foreach ([
        'Statement::$out_vars_count' => fn() => $q->out_vars_count,
        'Statement::$name' => fn() => $q->name,
        'Transaction::$id' => fn() => $t->id,
        'Blob::$total_length' => fn() => $b->total_length,
        'Blob::id()' => fn() => $b->id(),
    ] as $what => $read) {
        try {
            var_dump($read());
        } catch (\FireBird\Fb_Exception $e) {
            print "$what: " . $e->getMessage() . "\n";
        }
    }
})();
?>
--EXPECT--
string(0) ""
int(1)
bool(true)
bool(true)
Statement::$out_vars_count: Invalid database handle
Statement::$name: Invalid database handle
Transaction::$id: Invalid database handle
Blob::$total_length: Invalid database handle
Blob::id(): Invalid database handle
