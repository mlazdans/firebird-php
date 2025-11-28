<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once(__DIR__."/config.inc");

srand();

set_exception_handler(function(\Throwable $e){
    if ($e instanceof \FireBird\Fb_Exception) {
        print $e->getMessage()." in ".$e->getFile().":".$e->getLine();
        // print $e->getTraceAsString();
    } else {
        throw $e;
    }
});


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
    $res = $t->query("SELECT * from $table_name");
    while ($r = $res->fetch_row()) {
        echo join("\t",$r)."\t\n";
    }

    $res->free();

    echo "---\n";
}

function out_result(\FireBird\Statement $result, string $table_name = "")
{
    echo "--- $table_name ---\n";
    while ($r = $result->fetch_row()) {
        echo join("\t",$r)."\t\n";
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
    // "%m/%d/%Y %H:%M:%S"
    return sprintf("%4d-%02d-%02d %02d:%02d:%02d",
        rand()%100+1910, rand()%12+1, rand()%28+1,
        rand()%24,   rand()%60,  rand()%60);

    // return sprintf("%02d/%02d/%4d %02d:%02d:%02d",
    //     rand()%12+1, rand()%28+1, rand()%100+1910,
    //     rand()%24,   rand()%60,  rand()%60);
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
    return $prec ? $n : (int)$n;
}

// function print_error(string $cat): void
// {
//     hl();
//     if(isset($o->errors)){
//         printf("ERROR [$cat]:[$o->sqlstate] $o->error_file:$o->error_lineno\n");
//         if(!Config::$skip_ext_error && isset($o->ext_error_line)) {
//             print "at $o->ext_error_line\n";
//         }

//         foreach($o->errors as $i=>$e) {
//             printf("$i: $e->error_code_long ($e->error_code) $e->error_msg\n");
//         }
//     } else {
//         printf("ERROR [$cat] -no error information-\n");
//     }
//     hl();
// }

// function print_error_and_die(string $cat): never
// {
//     print_error($cat);
//     // debug_print_backtrace();
//     // print "===============================================================================\n";
//     die;
// }

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

function exec_from_file(\FireBird\Transaction $t, string $file_name): \FireBird\Statement
{
    return $t->query(load_file_or_die(Config::$pwd."/$file_name"));
}

function exec_from_file_ddl(\FireBird\Transaction $t, string $file_name): void
{
    exec_from_file($t, $file_name);
    $t->commit_ret();
}

function gen_random_db_file_name_or_die(): string
{
    srand();
    $database = Config::$tmp_root."/fbp_test".md5((string)rand());

    if(!empty(Config::$host))$database = Config::$host.":$database";

    $database .= ".fdb";

    return $database;
}

function args_apply_defaults(\FireBird\Create_Args|\FireBird\Connect_Args $args): \FireBird\Create_Args|\FireBird\Connect_Args
{
    if(!empty(Config::$user_name))$args->user_name = Config::$user_name;
    if(!empty(Config::$password))$args->password = Config::$password;

    if($args instanceof \FireBird\Create_Args) {
        if(!empty(Config::$charset))$args->set_db_charset = Config::$charset;
    } else {
        if(!empty(Config::$charset))$args->charset = Config::$charset;
    }

    return $args;
}

function init_tmp_db(string $db_file = "", string $charset = "", bool $drop = true): \FireBird\Database
{
    if(empty($db_file)){
        $args = new \FireBird\Create_Args;
        $args->database = gen_random_db_file_name_or_die();
        $args->page_size = 16 * 1024;
        $args->timeout = 10;
        args_apply_defaults($args);
        // overrides
        if($charset)$args->set_db_charset = $charset;
        if($db = \FireBird\Database::create($args)) {
            register_shutdown_function(function() use ($db, $drop) {
                $drop && $db->drop();
            });
            return $db;
        } else {
            die("could not create");
        }
    } else {
        $args = new \FireBird\Connect_Args;
        $args->database = Config::$host.":$db_file";
        args_apply_defaults($args);
        // overrides
        if($charset)$args->charset = $charset;

        if($db = \FireBird\Database::connect($args)) {
            register_shutdown_function(function() use ($db, $drop) {
                $drop && !$db->disconnect();
            });
            return $db;
        } else {
            die("could not connect");
        }
    }
}

function var_bin(mixed $v): void {
    ob_start();
    var_dump($v);
    $buff = ob_get_clean();

    $buff = replace_binary($buff);
    $buff = preg_replace(["/object\((.*)\)#\d+ \(\d+\) \{/m"], ["object($1) {"], $buff);

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

function run_tests(\FireBird\Transaction $t, array $select_tests): array|false
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
        } else {
            print "Query FAIL\n";
            return false;
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

// function query_or_error(\FireBird\Transaction $t, string $sql, ...$bind_args): \FireBird\Statement|false
// {
//     if(false === ($s = $t->query($sql, ...$bind_args))){
//         print_error("query", $t);
//         print_r(date_get_last_errors());
//     }
//     return $s;
// }

// function execute_immediate_or_error(\FireBird\Transaction $t, string $sql): bool
// {
//     if(false === $t->execute($sql)){
//         print_error("execute_immediate", $t);
//         print "Query: $sql\n";
//         return false;
//     }
//     return true;
// }

// function execute_immediate_or_die(\FireBird\Transaction $t, string $sql, ...$bind_args): bool
// {
//     if($s = execute_immediate_or_error($t, $sql, ...$bind_args)) {
//         return $s;
//     } else {
//         die;
//     }
// }

// function query_or_die(\FireBird\Transaction $t, string $sql, ...$bind_args): \FireBird\Statement
// {
//     if($s = query_or_error($t, $sql, ...$bind_args)) {
//         return $s;
//     } else {
//         print_r($sql);
//         die;
//     }
// }

// function prepare_or_error(\FireBird\Transaction $t, string $sql): \FireBird\Statement|false
// {
//     if(false === ($s = $t->prepare($sql))){
//         print_error("prepare", $t);
//     }
//     return $s;
// }

// function prepare_or_die(\FireBird\Transaction $t, string $sql): \FireBird\Statement
// {
//     if($s = prepare_or_error($t, $sql)) {
//         return $s;
//     } else {
//         die;
//     }
// }

function query_bulk(\FireBird\Transaction $t, array $queries): void
{
    foreach($queries as $query_tuple) {
        if(is_array($query_tuple)) {
            [$sql, $bind_args] = $query_tuple;
        } else {
            $sql = $query_tuple;
            $bind_args = [];
        }
        $t->query($sql, ...$bind_args);
    }
}

function execute_immediate_bulk(\FireBird\Transaction $t, array $queries, bool $verbose = false)
{
    foreach($queries as $sql) {
        if($verbose){
            print "$sql;\n";
        }

        $t->execute($sql);
    }
}

function fetch_and_print(\FireBird\Statement $q, int $flags = 0): void
{
    while($r = $q->fetch_object($flags)) {
        var_bin($r);
    }
}

function query_and_fetch_and_print(\FireBird\Transaction $t, string $sql, int $flags = 0): void
{
    fetch_and_print($t->query($sql), $flags);
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
    $q = $t->query('SELECT MON$ISOLATION_MODE FROM MON$TRANSACTIONS WHERE MON$TRANSACTION_ID = CURRENT_TRANSACTION');
    $row = $q->fetch_array();

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
    $t->execute("SET TRANSACTION ISOLATION LEVEL READ COMMITTED NO RECORD_VERSION");
    if(get_isolation_level_or_die($t) != 3) return true;
    $t->rollback();

    // 2
    $t = $db->new_transaction();
    $t->execute("SET TRANSACTION ISOLATION LEVEL READ COMMITTED RECORD_VERSION");
    if(get_isolation_level_or_die($t) != 2) return true;
    $t->rollback();

    return false;
}

function init_tmp_db_ibase(): \FireBird\Database
{
    $db = init_tmp_db();
    $t = $db->new_transaction();
    $t->start();
    $t->execute("CREATE TABLE TEST1 (I INTEGER, C VARCHAR(100))");
    $t->commit_ret();
    $t->query("INSERT INTO TEST1(I, C) VALUES(1, 'test table not created with isql')");
    $t->commit();
    // $db->disconnect();

    return $db;
}

function fields2sql_parts(array $fields): array
{
    $fields_str = '"'.join('","', $fields).'"';
    $q_str = join(",", array_fill(0, count($fields), "?"));
    return [$fields_str, $q_str];
}

function array2sql_parts(array $data): array
{
    return [...fields2sql_parts(array_keys($data)), $data];
}

function dump_rows(\FireBird\Statement $q, int $flags = 0): void
{
    while ($row = $q->fetch_array($flags)) {
        var_dump($row);
    }
}

function dump_table_rows(\FireBird\Transaction $t, $table, int $flags = 0): void
{
    $sql = sprintf('SELECT * FROM "%s"', $table);

    dump_rows($t->query($sql), $flags);
}

function skip_if_fb_lt(float $v): void
{
    if(($cv = get_fb_version()) < $v)die("skip Firebird server version $cv < $v");
}

function skip_if_fb_gt(float $v): void
{
    if(($cv = get_fb_version()) > $v)die("skip Firebird server version $cv > $v");
}

function skip_if_fb_gte(float $v): void
{
    if(($cv = get_fb_version()) >= $v)die("skip Firebird server version $cv >= $v");
}

// TODO: once service
function get_fb_version(): float
{
    die("TODO: once service");
    // global $host, $user, $password, $fb_version, $fb_version_str;

    // if(isset($fb_version)) return $fb_version;

    // if($se = ibase_service_attach($host ? $host : "localhost", $user, $password)) {
    //     $fb_version_str = ibase_server_info($se, IBASE_SVC_SERVER_VERSION);
    //     if($fb_version_str === false)die("skip cannot ibase_server_info()");
    //     $server_version  = explode(" ", $fb_version_str);
    //     $vers = (float)$server_version[count($server_version) - 1];
    //     return $fb_version = $vers;
    // } else {
    //     die("skip: cannot connect");
    // }
}

function insert_into(\FireBird\Transaction $t, string $table, array $data): \FireBird\Statement
{
    [$fields_str, $q_str] = array2sql_parts($data);
    $sql = sprintf('INSERT INTO "%s" (%s) VALUES (%s)', $table, $fields_str, $q_str);
    return $t->query($sql, ...array_values($data));
}
