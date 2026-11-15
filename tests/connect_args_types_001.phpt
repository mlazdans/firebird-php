--TEST--
Connect / create args: wrong classes and garbage are rejected, arg classes are final
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

use FireBird\Database;
use FireBird\Service;
use FireBird\Connect_Args;
use FireBird\Create_Args;
use FireBird\Service_Connect_Args;

require_once('functions.inc');

$garbage = [
    'Create_Args' => new Create_Args,
    'Connect_Args' => new Connect_Args,
    'Service_Connect_Args' => new Service_Connect_Args,
    'stdClass' => new \stdClass,
    'null' => null,
    'string' => "localhost:/tmp/x.fdb",
    'int' => 1,
    'array' => ['database' => "x"],
];

$calls = [
    'Database::connect' => fn($a) => Database::connect($a),
    'Database::create' => fn($a) => Database::create($a),
    'Service::connect' => fn($a) => Service::connect($a),
];

foreach ($calls as $call => $f) {
    foreach ($garbage as $name => $a) {
        try {
            $f($a);
            print "$call($name): accepted\n";
        } catch (\TypeError $e) {
            print "$call($name): ".get_class($e).": ".$e->getMessage()."\n";
        } catch (\Throwable $e) {
            print "$call($name): passed type check (".get_class($e).")\n";
        }
    }
}

foreach ([Connect_Args::class, Create_Args::class, Service_Connect_Args::class] as $c) {
    printf("%s final: %s\n", $c, var_export((new \ReflectionClass($c))->isFinal(), true));
}

?>
--EXPECTF--
Database::connect(Create_Args): TypeError: FireBird\Database::connect(): Argument #1 ($args) must be of type FireBird\Connect_Args, FireBird\Create_Args given
Database::connect(Connect_Args): passed type check (%s)
Database::connect(Service_Connect_Args): TypeError: FireBird\Database::connect(): Argument #1 ($args) must be of type FireBird\Connect_Args, FireBird\Service_Connect_Args given
Database::connect(stdClass): TypeError: FireBird\Database::connect(): Argument #1 ($args) must be of type FireBird\Connect_Args, stdClass given
Database::connect(null): TypeError: FireBird\Database::connect(): Argument #1 ($args) must be of type FireBird\Connect_Args, null given
Database::connect(string): TypeError: FireBird\Database::connect(): Argument #1 ($args) must be of type FireBird\Connect_Args, string given
Database::connect(int): TypeError: FireBird\Database::connect(): Argument #1 ($args) must be of type FireBird\Connect_Args, int given
Database::connect(array): TypeError: FireBird\Database::connect(): Argument #1 ($args) must be of type FireBird\Connect_Args, array given
Database::create(Create_Args): passed type check (%s)
Database::create(Connect_Args): TypeError: FireBird\Database::create(): Argument #1 ($args) must be of type FireBird\Create_Args, FireBird\Connect_Args given
Database::create(Service_Connect_Args): TypeError: FireBird\Database::create(): Argument #1 ($args) must be of type FireBird\Create_Args, FireBird\Service_Connect_Args given
Database::create(stdClass): TypeError: FireBird\Database::create(): Argument #1 ($args) must be of type FireBird\Create_Args, stdClass given
Database::create(null): TypeError: FireBird\Database::create(): Argument #1 ($args) must be of type FireBird\Create_Args, null given
Database::create(string): TypeError: FireBird\Database::create(): Argument #1 ($args) must be of type FireBird\Create_Args, string given
Database::create(int): TypeError: FireBird\Database::create(): Argument #1 ($args) must be of type FireBird\Create_Args, int given
Database::create(array): TypeError: FireBird\Database::create(): Argument #1 ($args) must be of type FireBird\Create_Args, array given
Service::connect(Create_Args): TypeError: FireBird\Service::connect(): Argument #1 ($args) must be of type FireBird\Service_Connect_Args, FireBird\Create_Args given
Service::connect(Connect_Args): TypeError: FireBird\Service::connect(): Argument #1 ($args) must be of type FireBird\Service_Connect_Args, FireBird\Connect_Args given
Service::connect(Service_Connect_Args): passed type check (%s)
Service::connect(stdClass): TypeError: FireBird\Service::connect(): Argument #1 ($args) must be of type FireBird\Service_Connect_Args, stdClass given
Service::connect(null): TypeError: FireBird\Service::connect(): Argument #1 ($args) must be of type FireBird\Service_Connect_Args, null given
Service::connect(string): TypeError: FireBird\Service::connect(): Argument #1 ($args) must be of type FireBird\Service_Connect_Args, string given
Service::connect(int): TypeError: FireBird\Service::connect(): Argument #1 ($args) must be of type FireBird\Service_Connect_Args, int given
Service::connect(array): TypeError: FireBird\Service::connect(): Argument #1 ($args) must be of type FireBird\Service_Connect_Args, array given
FireBird\Connect_Args final: true
FireBird\Create_Args final: true
FireBird\Service_Connect_Args final: true
