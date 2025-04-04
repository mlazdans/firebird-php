<?php declare(strict_types = 1);

namespace FireBirdTests;

class Config {
    static string $pwd          = __DIR__;
    static string $user_name    = 'SYSDBA';
    static string $password     = 'masterkey';
    static string $host         = "localhost/3050";
    static string $charset      = "utf8";
    static bool $skip_ext_error = true;
    static string $tmp_root     = "/tmp";             // Put your Firebird server tmp root here
}
