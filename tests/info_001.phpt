--TEST--
Info parsing: database info, SQLSTATE, row counts, limbo list, service users
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db("", true, false);
    $t = $db->start_transaction();

    print "--- Database::get_info() prints nothing and has the real creation date\n";
    ob_start();
    $info = $db->get_info();
    var_dump(ob_get_clean());
    $created = $t->query("SELECT MON\$CREATION_DATE FROM MON\$DATABASE")->fetch_row(\FireBird\FETCH_UNIXTIME)[0];
    var_dump($info->creation_date === $created, $info->page_size, $info->db_sql_dialect);

    print "--- SQLSTATE is five characters\n";
    try {
        $t->query("SELECT * FROM NO_SUCH_TABLE");
    } catch (\FireBird\Fb_Exception $e) {
        var_dump($e->sqlstate);
    }

    print "--- row counts\n";
    $t->execute("CREATE TABLE C (ID INTEGER)");
    $t->commit_ret();
    foreach ([1, 2, 3] as $i) $t->query("INSERT INTO C VALUES (?)", $i);
    $q = $t->query("UPDATE C SET ID = ID + 10 WHERE ID > 1");
    var_dump($q->update_count, $q->insert_count, $q->affected_count);

    print "--- limbo list\n";
    var_dump($db->get_limbo_transactions(1));

    print "--- service users\n";
    $args = new \FireBird\Service_Connect_Args;
    $args->service_name = Config::$host . ":service_mgr";
    $args->user_name = Config::$user_name;
    $args->password = Config::$password;
    $svc = \FireBird\Service::connect($args);
    $names = array_map(fn($u) => $u->username, $svc->get_users());
    var_dump(in_array("SYSDBA", $names, true));
    $svc->disconnect();

    $t->commit();
})();
?>
--EXPECT--
--- Database::get_info() prints nothing and has the real creation date
string(0) ""
bool(true)
int(16384)
int(3)
--- SQLSTATE is five characters
string(5) "42S02"
--- row counts
int(2)
int(0)
int(2)
--- limbo list
array(0) {
}
--- service users
bool(true)
