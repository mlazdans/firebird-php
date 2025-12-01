--TEST--
Get fbclient version
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

var_dump(
    \FireBird\get_client_version() === (float)\FireBird\get_client_major_version() + \FireBird\get_client_minor_version() / 10
);

?>
--EXPECT--
bool(true)
