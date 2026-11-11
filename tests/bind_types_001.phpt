--TEST--
Binding: NUMERIC scaling, floats, date/time strings, objects and zones
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
date.timezone=Europe/Riga
serialize_precision=-1
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

function try_bind(\FireBird\Transaction $t, string $sql, mixed ...$args): void
{
    try {
        $r = $t->query($sql, ...$args)->fetch_row();
        var_dump($r[0]);
    } catch (\Throwable $e) {
        print get_class($e) . ": " . $e->getMessage() . "\n";
    }
}

(function(){
    $t = init_tmp_db("", true, false)->start_transaction();

    print "--- NUMERIC(9,2)\n";
    $n = "SELECT CAST(? AS NUMERIC(9,2)) FROM RDB\$DATABASE";
    foreach (["1.234", "1.235", "-1.235", 5, "5", 5.5, 1234567.89, "1e3", " 12 ", 21474836, 21474837, "abc"] as $v) {
        try_bind($t, $n, $v);
    }

    print "--- NUMERIC(18,9) float precision\n";
    try_bind($t, "SELECT CAST(? AS NUMERIC(18,9)) FROM RDB\$DATABASE", 1234567.891234567);

    print "--- INTEGER / SMALLINT / BIGINT\n";
    try_bind($t, "SELECT CAST(? AS INTEGER) FROM RDB\$DATABASE", "42");
    try_bind($t, "SELECT CAST(? AS INTEGER) FROM RDB\$DATABASE", 7.0);
    try_bind($t, "SELECT CAST(? AS SMALLINT) FROM RDB\$DATABASE", 32768);
    try_bind($t, "SELECT CAST(? AS BIGINT) FROM RDB\$DATABASE", PHP_INT_MIN);
    try_bind($t, "SELECT CAST(? AS NUMERIC(18,2)) FROM RDB\$DATABASE", PHP_INT_MAX);
    try_bind($t, "SELECT CAST(? AS INTEGER) FROM RDB\$DATABASE", INF);
    try_bind($t, "SELECT CAST(? AS INTEGER) FROM RDB\$DATABASE", [1]);

    print "--- FLOAT / DOUBLE\n";
    try_bind($t, "SELECT CAST(? AS DOUBLE PRECISION) FROM RDB\$DATABASE", "1.5");
    try_bind($t, "SELECT CAST(? AS DOUBLE PRECISION) FROM RDB\$DATABASE", "abc");
    try_bind($t, "SELECT CAST(? AS FLOAT) FROM RDB\$DATABASE", 1e300);
    try_bind($t, "SELECT CAST(? AS DOUBLE PRECISION) FROM RDB\$DATABASE", NAN);

    print "--- date/time strings\n";
    $ts = "SELECT CAST(CAST(? AS TIMESTAMP) AS VARCHAR(40)) FROM RDB\$DATABASE";
    try_bind($t, $ts, "2025-11-06 13:45:59.1234");
    try_bind($t, $ts, "2025-11-06");
    try_bind($t, $ts, "not a date");
    try_bind($t, "SELECT CAST(CAST(? AS TIME) AS VARCHAR(40)) FROM RDB\$DATABASE", "13:45:59.5");
    try_bind($t, "SELECT CAST(CAST(? AS DATE) AS VARCHAR(40)) FROM RDB\$DATABASE", "2025-11-06");

    print "--- zones\n";
    $tz = "SELECT CAST(CAST(? AS TIMESTAMP WITH TIME ZONE) AS VARCHAR(60)) FROM RDB\$DATABASE";
    try_bind($t, $tz, "2025-07-01 12:00:00 Europe/Riga");
    try_bind($t, $tz, "2025-07-01 12:00:00 +05:30");
    try_bind($t, $tz, "2025-07-01 12:00:00 -03:00");
    try_bind($t, $tz, "2025-07-01 12:00:00 EEST");
    try_bind($t, $tz, "2025-07-01 12:00:00");
    try_bind($t, $tz, 0);
    try_bind($t, "SELECT CAST(CAST(? AS TIME WITH TIME ZONE) AS VARCHAR(60)) FROM RDB\$DATABASE", "13:45:59 +02:00");
    try_bind($t, "SELECT CAST(? AS TIME) FROM RDB\$DATABASE", 0);

    print "--- DateTime objects\n";
    try_bind($t, $tz, new \DateTimeImmutable("2025-07-01 12:00:00.5", new \DateTimeZone("America/New_York")));

    print "--- bound variables are not modified\n";
    $v = 5.5;
    $t->query("SELECT CAST(? AS VARCHAR(10)) FROM RDB\$DATABASE", $v);
    var_dump($v);

    print "--- NULL type parameter\n";
    try_bind($t, "SELECT 1 FROM RDB\$DATABASE WHERE ? IS NULL", null);
    try_bind($t, "SELECT 1 FROM RDB\$DATABASE WHERE ? IS NOT NULL", "x");

    $t->commit();
})();
?>
--EXPECTF--
--- NUMERIC(9,2)
string(4) "1.23"
string(4) "1.24"
string(5) "-1.24"
string(4) "5.00"
string(4) "5.00"
string(4) "5.50"
string(10) "1234567.89"
string(7) "1000.00"
string(5) "12.00"
string(11) "21474836.00"
FireBird\Fb_Exception: Argument 1: value 21474837 is out of range for NUMERIC(9,2)
FireBird\Fb_Exception: conversion error from string "abc"
--- NUMERIC(18,9) float precision
string(17) "1234567.891234567"
--- INTEGER / SMALLINT / BIGINT
int(42)
int(7)
FireBird\Fb_Exception: Argument 1: value 32768 is out of range for SMALLINT
int(-9223372036854775808)
FireBird\Fb_Exception: Argument 1: value 9223372036854775807 is out of range for NUMERIC(18,2)
FireBird\Fb_Exception: Argument 1: cannot bind INF or NAN to INTEGER
FireBird\Fb_Exception: Argument 1: type mismatch 'array' to SQL type 'INTEGER'
--- FLOAT / DOUBLE
float(1.5)
FireBird\Fb_Exception: Argument 1: 'abc' is not numeric
FireBird\Fb_Exception: Argument 1: value 1.0e+300 is out of range for FLOAT
FireBird\Fb_Exception: Argument 1: cannot bind INF or NAN to DOUBLE PRECISION
--- date/time strings
string(24) "2025-11-06 13:45:59.1234"
string(24) "2025-11-06 00:00:00.0000"
FireBird\Fb_Exception: Argument 1: could not parse 'not a date' as TIMESTAMP
string(13) "13:45:59.5000"
string(10) "2025-11-06"
--- zones
string(36) "2025-07-01 12:00:00.0000 Europe/Riga"
string(31) "2025-07-01 12:00:00.0000 +05:30"
string(31) "2025-07-01 12:00:00.0000 -03:00"
string(31) "2025-07-01 12:00:00.0000 +03:00"
string(36) "2025-07-01 12:00:00.0000 Europe/Riga"
string(31) "1970-01-01 00:00:00.0000 +00:00"
string(20) "13:45:59.0000 +02:00"
FireBird\Fb_Exception: Argument 1: type mismatch 'int' to SQL type 'TIME'
--- DateTime objects
string(41) "2025-07-01 12:00:00.5000 America/New_York"
--- bound variables are not modified
float(5.5)
--- NULL type parameter
int(1)
int(1)
