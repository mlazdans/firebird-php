<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once(__DIR__."/config.inc");

srand(((int)(double)microtime()*1000000));

function out_table($table_name)
{
    echo "--- $table_name ---\n";
    $res = ibase_query("select * from $table_name");
    while ($r = ibase_fetch_row($res)) {
        echo join("\t",$r)."\t\n";
    }
    ibase_free_result($res);
    echo "---\n";
}

function out_result($result, $table_name = "")
{
    echo "--- $table_name ---\n";
    while ($r = ibase_fetch_row($result)) {
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

function print_error(string $cat, \FireBird\IError $o)
{
    if(isset($o->error_code) && isset($o->error_code_long) && isset($o->error_msg)){
        printf("ERROR[$cat] ($o->error_code) ($o->error_code_long) on $o->error_file:$o->error_lineno\n");
        printf("$o->error_msg%s\n", isset($o->ext_error_line) ? "\n$o->ext_error_line" : "");
    } else {
        printf("ERROR[$cat]: -no error msg-\n");
    }
}

function print_error_and_die(...$args)
{
    print_error(...$args);
    die;
}

function query_from_file(\FireBird\Transaction $t, string $file_name): \FireBird\Statement|false
{
    $path = Config::$pwd."/$file_name";

    if(!is_file($path)) {
        die("ERROR: file not found $path");
    }

    if(!is_readable($path)) {
        die("ERROR: file is not readable $path");
    }

    if(false === ($sql = file_get_contents($path))) {
        die("ERROR: could not load file $path");
    }

    return $t->query($sql);
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

function init_tmp_db(): \FireBird\Connection|false
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

    $args = new \FireBird\Create_Args;
    $args->database = $database;
    if(!empty(Config::$user_name))$args->user_name = Config::$user_name;
    if(!empty(Config::$password))$args->password = Config::$password;
    if(!empty(Config::$charset))$args->set_db_charset = Config::$charset;

    $db = new \FireBird\Database;
    if($conn = $db->create($args)) {
        register_shutdown_function(function() use ($db) {
            if(!$db->drop()) {
                print_error_and_die("drop", $db);
            }
        });
        return $conn;
    } else {
        print_error_and_die("create", $db);
    }

    return false;
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
    foreach($select_tests as [$test_name, $sql, $args, $fetch_flags]) {
        $test_results = [];
        printf("%s\n", str_pad("$test_name ", 79, "-", STR_PAD_RIGHT));
        if($s = $t->query($sql, ...$args)) {
            print "Query OK\n";
            while($r = $s->fetch_object($fetch_flags)) {
                print "Row data ";
                var_bin($r);
                $test_results[] = $r;
            }

            if(false === $r) {
                print_error_and_die("fetch", $s);
            }
        } else {
            print_error_and_die("query", $t);
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
