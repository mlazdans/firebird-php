--TEST--
Database teardown: drop() and disconnect() with live statements, blobs and transactions
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

function new_db(): \FireBird\Database
{
    $args = new \FireBird\Create_Args;
    $args->database = gen_random_db_file_name_or_die();
    args_apply_defaults($args);
    return \FireBird\Database::create($args);
}

(function(){
    print "--- drop() with an open cursor and an unclosed blob\n";
    $db = new_db();
    $t = $db->start_transaction();
    $q = $t->query("SELECT 1 FROM RDB\$DATABASE UNION ALL SELECT 2 FROM RDB\$DATABASE");
    $q->fetch_row();
    $b = $t->create_blob();
    $b->put("x");
    $t->commit();
    $db->drop();
    print "dropped\n";

    print "--- disconnect() and drop() with an active transaction\n";
    $db = new_db();
    $t = $db->start_transaction();
    $t->query("SELECT 1 FROM RDB\$DATABASE");
    foreach (['disconnect', 'drop'] as $method) {
        try {
            $db->$method();
        } catch (\FireBird\Fb_Exception $e) {
            print $e->getMessage() . "\n";
        }
    }
    // The connection is still usable
    var_dump($t->query("SELECT 1 FROM RDB\$DATABASE")->fetch_row()[0]);
    $t->commit();
    $db->drop();
    print "dropped after commit\n";
})();
?>
--EXPECT--
--- drop() with an open cursor and an unclosed blob
dropped
--- disconnect() and drop() with an active transaction
Cannot disconnect with 1 active transaction(s): commit or roll back first
Cannot drop database with 1 active transaction(s): commit or roll back first
int(1)
dropped after commit
