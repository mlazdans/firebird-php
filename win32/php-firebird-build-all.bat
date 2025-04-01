@REM @echo off

@REM call php-firebird-build.bat 8.2 8.2.27 vs16 || exit /B %ERRORLEVEL%
@REM call php-firebird-build.bat 8.3 8.3.19 vs16 || exit /B %ERRORLEVEL%
call php-firebird-build.bat 8.4 8.4.4 vs17 || exit /B %ERRORLEVEL%
@REM call php-firebird-build.bat master vs17 || exit /B %ERRORLEVEL%
