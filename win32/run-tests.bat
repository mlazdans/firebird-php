@echo off

C:\php-sdk\php8.4\vs17\x64\php-src\x64\Release_TS\php.exe ^
C:\php-sdk\php8.4\vs17\x64\php-src\run-tests.php ../tests/ ^
--show-diff -n -q -d extension=..\..\releases\php_firebird-0.1.0-8.4-vs17-x86_64.dll -d error_log=../php-error.log

C:\php-sdk\php8.4\vs17\x64\php-src\x64\Release\php.exe ^
C:\php-sdk\php8.4\vs17\x64\php-src\run-tests.php ../tests/ ^
--show-diff -n -q -d extension=..\..\releases\php_firebird-0.1.0-8.4-vs17-nts-x86_64.dll -d error_log=../php-error.log

@REM C:\php-sdk\php8.3\vs16\x64\php-src\x64\Release\php.exe ^
@REM C:\php-sdk\php8.3\vs16\x64\php-src\run-tests.php ^
@REM --show-diff -n -q -d extension=..\releases\php_firebird-0.1.0-8.3-vs16-nts-x86_64.dll -d error_log=./php-error.log
