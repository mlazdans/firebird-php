--TEST--
php-firebird: ibase_drop_db(): Basic test
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $sql = sprintf("CREATE SCHEMA '%s' USER '%s' PASSWORD '%s' DEFAULT CHARACTER SET %s",
        gen_random_db_file_name_or_die(),
        Config::$user_name, Config::$password,
        Config::$charset ?? "NONE",
    );

    $db = \FireBird\Database::execute_create($sql);
    var_dump(get_class($db));

    $db->drop();
    var_dump(true);
    var_dump(isset($db->args));
})();

?>
--EXPECTF--
string(17) "FireBird\Database"
bool(true)
bool(false)
