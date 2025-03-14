<?php declare(strict_types = 1);

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

function init_tmp_db(string $table_file): \FireBird\Connection|false
{
    include('config.inc');

    $database = tempnam(sys_get_temp_dir(),"php_firebird_test");

    if($database === false) {
        die("Could not create temporary file: $database");
    }

    if(!unlink($database)) {
        die("Could not delete temporary file: $database");
    }

    if(!empty($host))$database = "$host:$database";

    $database .= ".fdb";

    $args = new \FireBird\Create_Args;
    $args->database = $database;
    if(!empty($user))$args->user_name = $user;
    if(!empty($password))$args->password = $password;
    if(!empty($charset))$args->set_db_charset = $charset;

    $db = new \FireBird\Database;
    if($conn = $db->create($args)) {
        if(($t = $conn->new_transaction())) {
            $sql = file_get_contents("$pwd/$table_file");
            if($t->start() && $t->query($sql)) {
                if($t->commit_ret()) {
                    register_shutdown_function(function() use ($db) {
                        if(!$db->drop()) {
                            print_error("drop", $db);
                        }
                    });
                    return $conn;
                } else {
                    print_error("commit", $t);
                }
            } else {
                print_error("query", $t);
            }
        } else {
            print_error("new_transaction", $conn);
        }
    } else {
        print_error("create", $db);
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
