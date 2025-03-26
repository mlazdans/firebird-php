<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once(__DIR__."/config.inc");

class Mon_Transaction
{
    public ?int $MON_TRANSACTION_ID;
    public ?int $MON_ATTACHMENT_ID;
    public ?int $MON_STATE;
    public ?string $MON_TIMESTAMP;
    public ?int $MON_TOP_TRANSACTION;
    public ?int $MON_OLDEST_TRANSACTION;
    public ?int $MON_OLDEST_ACTIVE;
    public ?int $MON_ISOLATION_MODE;
    public ?int $MON_LOCK_TIMEOUT;
    public ?int $MON_READ_ONLY;
    public ?int $MON_AUTO_COMMIT;
    public ?int $MON_AUTO_UNDO;
    public ?int $MON_STAT_ID;
}

function out_table(\FireBird\Transaction $t, string $table_name)
{
    echo "--- $table_name ---\n";
    $res = $t->query("SELECT * from $table_name") or print_error_and_die("query", $t);
    while ($r = $res->fetch_row()) {
        echo join("\t",$r)."\t\n";
    }

    if($r === false) print_error_and_die("fetch", $res);

    $res->close() or print_error_and_die("close", $res);

    echo "---\n";
}

function out_result(\FireBird\Statement $result, string $table_name = "", bool $close = true)
{
    echo "--- $table_name ---\n";
    while ($r = $result->fetch_row()) {
        echo join("\t",$r)."\t\n";
    }

    if($r === false) print_error_and_die("fetch", $result);

    if($close) {
        $result->close() or print_error_and_die("close", $result);
    }

    echo "---\n";
}

function out_result_trap_error($result, $table_name = "")
{
   echo "--- $table_name ---\n";
   while ($r = @ibase_fetch_row($result)) {
        echo join("\t",$r)."\t\n";
   }
   echo "errmsg [" . ibase_errmsg() . "]\n";
   echo "---\n";
}

/* M/D/Y H:M:S */
function rand_datetime()
{
    return sprintf("%02d/%02d/%4d %02d:%02d:%02d",
        rand()%12+1, rand()%28+1, rand()%100+1910,
        rand()%24,   rand()%60,  rand()%60);
}

/* random binary string  */
function rand_binstr($max_len)
{
    $len = rand() % $max_len;
    $s = "";
    while($len--) {
        $s .= sprintf("%c", rand() % 256);
    }
    return $s;
}

function rand_str($max_len)
{
    $len = rand() % $max_len;
    $s = "";
    while ($len--) {
        $s .= sprintf("%c", rand() % 26 + 65);
    }
    return $s;
}

function rand_number($len , $prec = -1, $sign = 1)
{
    if ($prec == -1) {
        $n = substr(rand() . rand(), 0, rand() % $len + 1);
        if (strlen($n) < $len) {
            $n .= "." . substr((string)rand(), 0, rand() % ($len - strlen($n)) + 1);
        }
    } else if ($prec == 0) {
        $n = substr(rand() . rand(), 0, rand() % $len + 1);
    } else if (($prec - $len) == 0) {
        $n = substr(rand() . rand(), 0, 1);
        $n .= "." . substr((string)rand(), 0, $prec);
    } else {
        $n = substr(rand() . rand(), 0, rand() % ($len - $prec) + 1);
        $n .= "." . substr((string)rand(), 0, $prec);
    }
    if ($sign && (rand() % 3 == 0)) {
        $n = "-" .$n;
    }
    return $n;
}

function print_error(string $cat, \FireBird\IError $o): void
{
    hl();
    if(isset($o->errors)){
        printf("ERROR [$cat]:[$o->sqlstate] $o->error_file:$o->error_lineno\n");
        if(!Config::$skip_ext_error && isset($o->ext_error_line)) {
            print "at $o->ext_error_line\n";
        }

        foreach($o->errors as $i=>$e) {
            printf("$i: $e->error_code_long ($e->error_code) $e->error_msg\n");
        }
    } else {
        printf("ERROR [$cat] -no error information-\n");
    }
    hl();
}

function print_error_and_die(string $cat, \FireBird\IError $o): never
{
    print_error($cat, $o);
    // debug_print_backtrace();
    // print "===============================================================================\n";
    die;
}

function load_file_or_die(string $path): string
{
    if(!is_file($path)) {
        die("ERROR: file not found $path");
    }

    if(!is_readable($path)) {
        die("ERROR: file is not readable $path");
    }

    if(false === ($data = file_get_contents($path))) {
        die("ERROR: could not load file $path");
    }

    return $data;
}

function query_from_file(\FireBird\Transaction $t, string $file_name): \FireBird\Statement|false
{
    return $t->query(load_file_or_die(Config::$pwd."/$file_name"));
}

function query_from_file_ddl(\FireBird\Transaction $t, string $file_name): \FireBird\Statement
{
    if(false === ($s = query_from_file($t, $file_name))) {
        print_error_and_die("query_from_file_ddl", $t);
    }

    if(!$t->commit_ret()) {
        print_error_and_die("commit_ret", $t);
    }

    return $s;
}

function gen_random_db_file_name_or_die(): string
{
    $database = tempnam(sys_get_temp_dir(),"php_firebird_test");

    if($database === false) {
        die("Could not create temporary file: $database");
    }

    if(!unlink($database)) {
        die("Could not delete temporary file: $database");
    }

    if(!empty(Config::$host))$database = Config::$host.":$database";

    $database .= ".fdb";

    return $database;
}

function args_apply_defaults(\FireBird\Create_Args|\FireBird\Connect_Args  $args): void {
    if(!empty(Config::$user_name))$args->user_name = Config::$user_name;
    if(!empty(Config::$password))$args->password = Config::$password;

    if($args instanceof \FireBird\Create_Args) {
        if(!empty(Config::$charset))$args->set_db_charset = Config::$charset;
    } else {
        if(!empty(Config::$charset))$args->charset = Config::$charset;
    }
}

function init_tmp_db(string $db_file = "", string $charset = "", bool $drop = true): \FireBird\Database
{
    $db = new \FireBird\Database;

    if(empty($db_file)){
        $args = new \FireBird\Create_Args;
        $args->database = gen_random_db_file_name_or_die();
        $args->page_size = 16 * 1024;
        $args->timeout = 10;
        args_apply_defaults($args);
        // overrides
        if($charset)$args->set_db_charset = $charset;
        if($db->create($args)) {
            register_shutdown_function(function() use ($db, $drop) {
                if($drop && !$db->drop()) {
                    print_error_and_die("drop", $db);
                }
            });
            return $db;
        }
        print_error_and_die("create", $db);
    } else {
        $args = new \FireBird\Connect_Args;
        $args->database = Config::$host.":$db_file";
        args_apply_defaults($args);
        // overrides
        if($charset)$args->charset = $charset;

        if($db->connect($args)) {
            register_shutdown_function(function() use ($db, $drop) {
                if($drop && !$db->disconnect()) {
                    print_error_and_die("disconnect", $db);
                }
            });
            return $db;
        }
        print_error_and_die("connect", $db);
    }
}

function var_bin(mixed $v): void {
    ob_start();
    var_dump($v);
    $buff = ob_get_clean();

    $buff = replace_binary($buff);
    $buff = preg_replace(["/object\(stdClass\)#\d+ \(\d+\) \{/m"], ["object {"], $buff);

    print $buff;
}

function replace_binary(string $data): string
{
    $buff = preg_replace_callback(
        '/[^[:print:]\s\n]/u',
        function ($match) {
            return sprintf("\\x%02X", ord($match[0]));
        },
        $data
    );

    return $buff ?? $data;
}

function run_tests(\FireBird\Transaction $t, array $select_tests): array
{
    $results = [];
    foreach($select_tests as $i=>[$test_name, $sql, $bind_args, $fetch_flags]) {
        $test_results = [];
        printf("%s\n", str_pad("$test_name ", 79, "-", STR_PAD_RIGHT));
        if($s = $t->query($sql, ...$bind_args)) {
            print "Query OK\n";
            while($r = $s->fetch_object($fetch_flags)) {
                print "Row data ";
                var_bin($r);
                $test_results[] = $r;
            }

            if(false === $r) {
                print_error_and_die("fetch [$i]", $s);
            }
        } else {
            print_error_and_die("query [$i]", $t);
        }
        print "\n";

        $results[] = $test_results;
    }

    return $results;
}

function results_equal(array $test_result1, array $test_result2): bool
{
    $f = function(object $o1, object $o2): int {
        return $o1 == $o2 ? 0 : -1;
    };

    return array_udiff($test_result1, $test_result2, $f) == [];
}

function query_or_error(\FireBird\Transaction $t, string $sql, ...$bind_args): \FireBird\Statement|false
{
    if(false === ($s = $t->query($sql, ...$bind_args))){
        print_error("query", $t);
    }
    return $s;
}

function query_or_die(\FireBird\Transaction $t, string $sql, ...$bind_args): \FireBird\Statement
{
    if($s = query_or_error($t, $sql, ...$bind_args)) {
        return $s;
    } else {
        die;
    }
}

function prepare_or_error(\FireBird\Transaction $t, string $sql): \FireBird\Statement|false
{
    if(false === ($s = $t->prepare($sql))){
        print_error("prepare", $t);
    }
    return $s;
}

function prepare_or_die(\FireBird\Transaction $t, string $sql): \FireBird\Statement
{
    if($s = prepare_or_error($t, $sql)) {
        return $s;
    } else {
        die;
    }
}

function query_bulk(\FireBird\Transaction $t, array $queries): bool
{
    foreach($queries as $query_tuple) {
        if(is_array($query_tuple)) {
            [$sql, $bind_args] = $query_tuple;
        } else {
            $sql = $query_tuple;
            $bind_args = [];
        }
        if(!query_or_error($t, $sql, ...$bind_args)){
            return false;
        }
    }
    return true;
}

function query_bulk_or_die(\FireBird\Transaction $t, array $queries): void
{
    if(!query_bulk($t, $queries)){
        die;
    }
}

function fetch_and_print(\FireBird\Statement $q, int $flags = 0): void
{
    while($r = $q->fetch_object($flags)) {
        var_bin($r);
    }

    if(false === $r) {
        print_error("fetch", $q);
    }
}

function fetch_and_print_or_die(\FireBird\Statement $q, int $flags = 0): void
{
    while($r = $q->fetch_object($flags)) {
        var_bin($r);
    }

    if(false === $r) {
        print_error_and_die("fetch", $q);
    }
}

function query_and_fetch_and_print_or_die(\FireBird\Transaction $t, string $sql, int $flags = 0): void
{
    fetch_and_print_or_die(query_or_die($t, $sql), $flags);
}

function hl(?string $msg = null): void
{
    print str_pad($msg ? " $msg " : "", 80, "=", STR_PAD_BOTH)."\n";
}

function unstrip_fb_specials(array &$data)
{
    foreach($data as $k=>$v) {
        if(false === ($pos = strpos($k, '$'))) continue;
        unset($data[$k]);
        $new_k = str_replace('$', '_', $k);
        $data[$new_k] = $v;
    }
}

function cmp_builder(\FireBird\TBuilder $b, array $mon_row, bool $is_ReadConsistency_enabled): bool
{
    /** @var Mon_Transaction $m */
    $m = (object)$mon_row;

    $error = false;

    if($b->is_read_only !== (bool)$m->MON_READ_ONLY){
        $error = true;
        printf("Transaction param is_read_only: [%d] does not match actual [%d]\n", $b->is_read_only, $m->MON_READ_ONLY);
    }

    if($b->is_auto_commit !== (bool)$m->MON_AUTO_COMMIT){
        $error = true;
        printf("Transaction param is_auto_commit: [%d] does not match actual [%d]\n", $b->is_auto_commit, $m->MON_AUTO_COMMIT);
    }

    if($b->is_no_auto_undo === (bool)$m->MON_AUTO_UNDO){
        $error = true;
        printf("Transaction param is_no_auto_undo: [%d] does not match actual [%d]\n", $b->is_no_auto_undo, !$m->MON_AUTO_UNDO);
    }

    if($is_ReadConsistency_enabled && ($b->isolation_mode === 2 || $b->isolation_mode === 3)) {
        if($m->MON_ISOLATION_MODE !== 4){
            $error = true;
            printf("Transaction param isolation_mode: 4 does not match actual [%d]\n", $m->MON_ISOLATION_MODE);
        }

    } else {
        if($b->isolation_mode !== $m->MON_ISOLATION_MODE){
            $error = true;
            printf("Transaction param isolation_mode: [%d] does not match actual [%d]\n", $b->isolation_mode, $m->MON_ISOLATION_MODE);
        }
    }

    if($b->lock_timeout !== $m->MON_LOCK_TIMEOUT){
        $error = true;
        printf("Transaction param lock_timeout: [%d] does not match actual [%d]\n", $b->lock_timeout, $m->MON_LOCK_TIMEOUT);
    }

    return $error;
}

function random_builder(): \FireBird\TBuilder
{
    $b = new \FireBird\TBuilder;

    if(rand(0, 1))$b->read_only();
    if(rand(0, 1))$b->ignore_limbo();
    if(rand(0, 1))$b->auto_commit();
    if(rand(0, 1))$b->no_auto_undo();

    $f = [
        'isolation_snapshot',
        'isolation_snapshot_table_stability',
        'isolation_read_committed_record_version',
        'isolation_read_committed_no_record_version',
        'isolation_read_committed_read_consistency',
    ];
    $b->{$f[rand(0, count($f) - 1)]}();

    if(rand(0, 1)) {
        $b->wait(0);
    } else {
        $b->wait(rand(1, 0x7FFF));
    }

    return $b;
}

function get_isolation_level_or_die(\FireBird\Transaction $t): int
{
    $q = $t->query('SELECT MON$ISOLATION_MODE FROM MON$TRANSACTIONS WHERE MON$TRANSACTION_ID = CURRENT_TRANSACTION') or print_error_and_die("query", $t);
    $row = $q->fetch_array() or print_error_and_die("fetch", $q);

    return $row['MON$ISOLATION_MODE'];
}

// ReadConsistency = 0 in firebird.conf modifies behaviour of these:
// 2 - read committed record version
// 3 - read committed no record version
// Both of these levels are set to 4 (read committed read consistency)
function is_ReadConsistency_enabled(\FireBird\Database $db): bool
{
    // 3
    $t = $db->new_transaction();
    $t->query("SET TRANSACTION ISOLATION LEVEL READ COMMITTED NO RECORD_VERSION") or print_error_and_die("set trans", $t);
    if(get_isolation_level_or_die($t) != 3) return true;
    $t->rollback() or print_error_and_die("rollback", $t);

    // 2
    $t = $db->new_transaction();
    $t->query("SET TRANSACTION ISOLATION LEVEL READ COMMITTED RECORD_VERSION") or print_error_and_die("set trans", $t);
    if(get_isolation_level_or_die($t) != 2) return true;
    $t->rollback() or print_error_and_die("rollback", $t);

    return false;
}
