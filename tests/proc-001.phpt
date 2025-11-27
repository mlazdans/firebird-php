--TEST--
Procedures
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $t = init_tmp_db_ibase()->start_transaction();

    $t->query(
    "CREATE OR ALTER PROCEDURE GET_5_RECORDS(ARG INTEGER)
    RETURNS (N INTEGER, RESULT INTEGER)
    AS
    DECLARE VARIABLE I INTEGER;
    BEGIN
        I = 1;
        WHILE (:I <= 5) DO BEGIN
            N = :I;
            RESULT = :ARG + :I;
            I = :I + 1;
            SUSPEND;
        END
    END");

    $q = $t->prepare("EXECUTE PROCEDURE GET_5_RECORDS(?)");

    $q->execute(1); dump_rows($q);
    $q->execute(10); dump_rows($q);

    print "------------------\n";

    $q = $t->prepare("SELECT * FROM GET_5_RECORDS(?)");
    $q->execute(1); dump_rows($q);
    $q->execute(10); dump_rows($q);
})();

?>
--EXPECT--
array(2) {
  ["N"]=>
  int(1)
  ["RESULT"]=>
  int(2)
}
array(2) {
  ["N"]=>
  int(1)
  ["RESULT"]=>
  int(11)
}
------------------
array(2) {
  ["N"]=>
  int(1)
  ["RESULT"]=>
  int(2)
}
array(2) {
  ["N"]=>
  int(2)
  ["RESULT"]=>
  int(3)
}
array(2) {
  ["N"]=>
  int(3)
  ["RESULT"]=>
  int(4)
}
array(2) {
  ["N"]=>
  int(4)
  ["RESULT"]=>
  int(5)
}
array(2) {
  ["N"]=>
  int(5)
  ["RESULT"]=>
  int(6)
}
array(2) {
  ["N"]=>
  int(1)
  ["RESULT"]=>
  int(11)
}
array(2) {
  ["N"]=>
  int(2)
  ["RESULT"]=>
  int(12)
}
array(2) {
  ["N"]=>
  int(3)
  ["RESULT"]=>
  int(13)
}
array(2) {
  ["N"]=>
  int(4)
  ["RESULT"]=>
  int(14)
}
array(2) {
  ["N"]=>
  int(5)
  ["RESULT"]=>
  int(15)
}