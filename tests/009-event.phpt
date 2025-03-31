--TEST--
FireBird: events
--SKIPIF--
<?php

require_once('functions.inc');
include("skipif.inc");
if (strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') print "skip Broken on windows\n";

?>
--FILE--
<?php declare(strict_types = 1);

namespace FireBirdTests;

require_once('functions.inc');

(function(){
    $db = init_tmp_db();

    ($t = $db->new_transaction())->start() or print_error_and_die("start tr", $db);

    $t->execute_immediate(load_file_or_die(Config::$pwd."/009-post_event.sql")) or print_error_and_die("create proc", $t);
    $t->commit_ret();

    $f1 = function(int $count) {
        print "=== Hello from PHP1 === $count\n";
        return true;
    };

    $f2 = function(int $count) {
        print "*** Hello from PHP2 *** $count\n";
        return true;
    };

    $db->on_event("TEST1", $f1) or print_error_and_die("on_event1", $db);
    $db->on_event("TEST2", $f2) or print_error_and_die("on_event1", $db);

    $t->execute_immediate("EXECUTE PROCEDURE PROC_EVENT") or print_error_and_die("execute_immediate", $t);

    sleep(1);

    \FireBird\Event::consume();
})();

?>
--EXPECT--
*** Hello from PHP2 *** 1
=== Hello from PHP1 === 1
