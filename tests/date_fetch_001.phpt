--TEST--
Fetching date/time columns: DateTime objects by default, FETCH_UNIXTIME
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
date.timezone=Europe/Riga
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db("", true, false)->start_transaction();

    // Summer time, fractions, an offset zone and the hour repeated when summer time ends
    $sql = "SELECT
        CAST('2025-07-01' AS DATE) D,
        CAST('13:45:59.1234' AS TIME) T,
        CAST('13:45:59.1234 Europe/Riga' AS TIME WITH TIME ZONE) TZ,
        CAST('2025-07-01 13:45:59.1234' AS TIMESTAMP) TS,
        CAST('2025-07-01 13:45:59.1234 Europe/Riga' AS TIMESTAMP WITH TIME ZONE) TSZ,
        CAST('2025-01-01 00:00:00 +05:30' AS TIMESTAMP WITH TIME ZONE) OFS,
        CAST('2025-10-26 03:30:00 Europe/Riga' AS TIMESTAMP WITH TIME ZONE) DST_END
        FROM RDB\$DATABASE";

    foreach (['default' => 0, 'FETCH_UNIXTIME' => \FireBird\FETCH_UNIXTIME] as $name => $flags) {
        print "--- $name\n";
        foreach ($t->query($sql)->fetch_array($flags) as $k => $v) {
            print "$k: " . (is_object($v) ? $v->format('Y-m-d H:i:s.u P e') . " ts=" . $v->getTimestamp() : var_export($v, true)) . "\n";
        }
    }

    print "--- fetched DateTime objects bind back unchanged\n";
    $r = $t->query("SELECT CAST('2025-07-01 13:45:59.1234 Europe/Riga' AS TIMESTAMP WITH TIME ZONE) FROM RDB\$DATABASE")->fetch_row();
    var_dump($t->query("SELECT CAST(CAST(? AS TIMESTAMP WITH TIME ZONE) AS VARCHAR(60)) FROM RDB\$DATABASE", $r[0])->fetch_row()[0]);

    print "--- each value is its own object\n";
    $q = $t->query("SELECT CAST('2025-01-01' AS DATE) FROM RDB\$DATABASE UNION ALL SELECT CAST('2025-07-01' AS DATE) FROM RDB\$DATABASE");
    $a = $q->fetch_row()[0];
    $b = $q->fetch_row()[0];
    print $a->format('Y-m-d P') . " " . $b->format('Y-m-d P') . "\n";

    $t->commit();
})();
?>
--EXPECT--
--- default
D: 2025-07-01 00:00:00.000000 +03:00 Europe/Riga ts=1751317200
T: 1970-01-01 13:45:59.123400 +03:00 Europe/Riga ts=38759
TZ: 1970-01-01 13:45:59.123400 +03:00 Europe/Riga ts=38759
TS: 2025-07-01 13:45:59.123400 +03:00 Europe/Riga ts=1751366759
TSZ: 2025-07-01 13:45:59.123400 +03:00 Europe/Riga ts=1751366759
OFS: 2025-01-01 00:00:00.000000 +05:30 +05:30 ts=1735669800
DST_END: 2025-10-26 03:30:00.000000 +03:00 Europe/Riga ts=1761438600
--- FETCH_UNIXTIME
D: 1751328000
T: 1970-01-01 13:45:59.123400 +03:00 Europe/Riga ts=38759
TZ: 1970-01-01 13:45:59.123400 +03:00 Europe/Riga ts=38759
TS: 1751377559
TSZ: 1751366759
OFS: 1735669800
DST_END: 1761438600
--- fetched DateTime objects bind back unchanged
string(36) "2025-07-01 13:45:59.1234 Europe/Riga"
--- each value is its own object
2025-01-01 +02:00 2025-07-01 +03:00
