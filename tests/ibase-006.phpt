--TEST--
InterBase: binding (may take a while). Ported from php-firebird.
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    if(false === ($conn = init_tmp_db(charset: "NONE"))) {
        return;
    }

    if(!(($t = $conn->new_transaction()) && $t->start())) {
        print_error_and_die("transaction", $conn);
    }

    query_or_die($t,
        "CREATE table test6 (
            iter		integer,
            v_char		char(1000),
            v_date      timestamp,
            v_decimal   decimal(12,3),
            v_double  	double precision,
            v_float     float,
            v_integer   integer,
            v_numeric   numeric(4,2),
            v_smallint  smallint,
            v_varchar   varchar(10000)
            )");

    query_or_die($t,
        "CREATE procedure add1 (arg integer)
        returns (result integer)
        as
        begin
            result = arg +1;
        end");

    $t->commit_ret() or print_error_and_die("commit_ret", $t);

    /* if timefmt not supported, hide error */
    ini_set('firebird.timestampformat',"%m/%d/%Y %H:%M:%S");

    echo "insert\n";

    for($iter = 0; $iter < 3; $iter++) {
        /* prepare data  */
        $v_char = rand_str(1000);
        $v_date = rand_datetime();
        $v_decimal = rand_number(12,3);
        $v_double  = rand_number(20);
        $v_float   = rand_number(7);
        $v_integer = rand_number(9,0);
        $v_numeric = rand_number(4,2);
        $v_smallint = ((int)rand_number(5)) % 32767;
        $v_varchar = rand_str(10000);

        query_or_die($t, "INSERT into test6
            (iter,v_char,v_date,v_decimal,v_double,v_float,
            v_integer,v_numeric,v_smallint,v_varchar)
            values (?,?,?,?,?,?,?,?,?,?)",
            $iter, $v_char, $v_date, $v_decimal, $v_double, $v_float,
            $v_integer, $v_numeric, $v_smallint, $v_varchar);

        $sel = query_or_die($t, "SELECT * from test6 where iter = ?", $iter);

        $row = $sel->fetch_object();
        if(false === $row) print_error_and_die("fetch_object", $sel);

        if(substr($row->V_CHAR,0,strlen($v_char)) != $v_char) {
            echo " CHAR fail:\n";
            echo " in:  $v_char\n";
            echo " out: $row->V_CHAR\n";
        }
        if($row->V_DATE != $v_date) {
            echo " DATE fail\n";
            echo " in:  $v_date\n";
            echo " out: $row->V_DATE\n";
        }
        if($row->V_DECIMAL != $v_decimal) {
            echo " DECIMAL fail\n";
            echo " in:  $v_decimal\n";
            echo " out: $row->V_DECIMAL\n";
        }
        if(abs($row->V_DOUBLE - $v_double) > abs($v_double / 1E15)) {
            echo " DOUBLE fail\n";
            echo " in:  $v_double\n";
            echo " out: $row->V_DOUBLE\n";
        }
        if(abs($row->V_FLOAT - $v_float) > abs($v_float / 1E7)) {
            echo " FLOAT fail\n";
            echo " in:  $v_float\n";
            echo " out: $row->V_FLOAT\n";
        }
        if($row->V_INTEGER != $v_integer) {
            echo " INTEGER fail\n";
            echo " in:  $v_integer\n";
            echo " out: $row->V_INTEGER\n";
        }
        if ($row->V_NUMERIC != $v_numeric) {
            echo " NUMERIC fail\n";
            echo " in:  $v_numeric\n";
            echo " out: $row->V_NUMERIC\n";
        }
        if ($row->V_SMALLINT != $v_smallint) {
            echo " SMALLINT fail\n";
            echo " in:  $v_smallint\n";
            echo " out: $row->V_SMALLINT\n";
        }
        if ($row->V_VARCHAR != $v_varchar) {
            echo " VARCHAR fail:\n";
            echo " in:  $v_varchar\n";
            echo " out: $row->V_VARCHAR\n";
        }
        $sel->close() or print_error_and_die("close", $sel);
    }/* for($iter)*/

    echo "select\n";
    for($iter = 0; $iter < 3; $iter++) {
        /* prepare data  */
        $v_char = rand_str(1000);
        $v_date = (int)rand_number(10,0,0);
        $v_decimal = rand_number(12,3);
        $v_double  = rand_number(20);
        $v_float   = rand_number(7);
        $v_integer = rand_number(9,0);
        $v_numeric = rand_number(4,2);
        $v_smallint = ((int)rand_number(5)) % 32767;
        $v_varchar = rand_str(10000);

        /* clear table*/
        query_or_die($t, "DELETE from test6");

        /* make one record */
        query_or_die($t, "INSERT into test6
            (iter, v_char,v_date,v_decimal,
            v_integer,v_numeric,v_smallint,v_varchar)
            values (666, '$v_char',?,$v_decimal, $v_integer,
            $v_numeric, $v_smallint, '$v_varchar')",$v_date);

        $test_f = function(\FireBird\Transaction $t, string $msg, string $sql, ...$bind_args) {
            $sel = query_or_die($t, $sql, ...$bind_args);
            if(!$sel->fetch_row())echo "$msg\n";
            $sel->close() or print_error_and_die("close", $sel);
        };

        /* test all types */
        $test_f($t, "CHAR fail", "SELECT iter from test6 where v_char = ?", $v_char);
        $test_f($t, "DATE fail", "SELECT iter from test6 where v_date = ?", $v_date);
        $test_f($t, "DECIMAL fail", "SELECT iter from test6 where v_decimal = ?", $v_decimal);
        $test_f($t, "INTEGER fail", "SELECT iter from test6 where v_integer = ?", $v_integer);
        $test_f($t, "NUMERIC fail", "SELECT iter from test6 where v_numeric = ?", $v_numeric);
        $test_f($t, "SMALLINT fail", "SELECT iter from test6 where v_smallint = ?", $v_smallint);
        $test_f($t, "VARCHAR fail", "SELECT iter from test6 where v_varchar = ?", $v_varchar);
    } /*for iter*/

    echo "prepare and exec insert\n";

    /* prepare table */
    query_or_die($t, "DELETE from test6");

    /* prepare query */
    $query = prepare_or_die($t, "INSERT into test6 (v_integer) values (?)");

    for($i = 0; $i < 10; $i++) {
        $query->execute($i) or print_error_and_die("execute", $query);
    }

    out_table($t, "test6");

    echo "prepare and exec select\n";

    /* prepare query */
    $query = prepare_or_die($t, "SELECT * from test6
        where v_integer between ? and ?");

    $low_border = 2;
    $high_border = 6;

    $query->execute($low_border, $high_border) or print_error_and_die("execute", $query);
    out_result($query, "test6");
    // $query->free() or print_error_and_die("free", $query);

    $low_border = 0;
    $high_border = 4;
    $query->execute($low_border, $high_border) or print_error_and_die("execute", $query);
    out_result($query, "test6");
    // $query->free() or print_error_and_die("free", $query);

    $query->execute("5", 7.499) or print_error_and_die("execute", $query);
    out_result($query, "test6");

    $query->free() or print_error_and_die("free", $query);

    /* test execute procedure */
    $query = prepare_or_die($t, "execute procedure add1(?)");

    // XXX: collecting execute results on same query does not makes sense.
    // Code below only works only because of some bug / feature of php-firebird.
    // Repeated ibase_execute() call will just close already opened cursor,
    // except for procedures

    // $res = array();
    // for ($i = 0; $i < 10; $i++) {
    //     $res[] = ibase_execute($query,$i);
    // }
    // ibase_free_query($query);
    // foreach ($res as $r) {
    //     out_result($r, "proc add1");
    //     ibase_free_result($r);
    // }

    for ($i = 0; $i < 10; $i++) {
        $query->execute($i) or print_error_and_die("execute", $query);
        out_result($query, "proc add1", false); // EXECUTE PROCEDURE has no cursor, so nothing to close
    }

    $t->commit() or print_error_and_die("commit", $t);

    echo "end of test\n";
})();
?>
--EXPECT_EXTERNAL--
ibase-006.out.txt
