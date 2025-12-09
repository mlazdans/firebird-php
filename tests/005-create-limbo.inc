<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function() {
    $share_path = sys_get_temp_dir().DIRECTORY_SEPARATOR."firebird-php-005-db.txt";

    if(file_exists($share_path)) {
        unlink($share_path);
        // die("skip File $database_share_path already exists. Manual cleanup recommended.");
    }

    $db = init_tmp_db(drop: false);

    $tr_id = (function() use ($db): int {
        // Create table
        $t = $db->start_transaction();
        // exec_from_file_ddl($t, "001-table.sql");

        // First insert some data via standard transaction
        $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Bogus mogus");
        $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Foo bar");
        $t->commit();

        // Insert more data and put transaction in limbo
        $t = $db->start_transaction();
        $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Hello from transaction $t->id");
        $t->prepare_2pc();

        print "Transaciton ID: $t->id prepared for two-phase commit\n";

        // These both inserts should fail because transaction has been prepared for 2pc
        try {
            $t->query("INSERT INTO TEST_001 (BLOB_1) VALUES (?)", "Hello from insert after 2pc transaction $t->id");
            die2("No insert after prepare_2pc should happen!");
        } catch (\FireBird\Fb_Exception) {
        }

        if($p = $t->prepare("INSERT INTO TEST_001 (BLOB_1) VALUES (?)")) {
            try {
                if($p->execute("Hello from prepare after 2pc transaction $t->id")) {
                    die2("No prepare after prepare_2pc should happen!");
                }
            } catch (\FireBird\Fb_Exception) {
            }
            $p->free();
        }

        return $t->id;
    })();

    file_put_contents($share_path, $db->args->database."\n$tr_id");
})();
