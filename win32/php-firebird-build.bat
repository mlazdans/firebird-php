@echo off

@REM config  ======================================================================================
call php-firebird-config.bat

goto :MAIN

@REM log  =========================================================================================
@REM log <msg>
@REM example> call :log "<msg>"
:log
    set msg=%~1
    echo ---------------------------------------------------------------------
    echo %msg%
    echo ---------------------------------------------------------------------
exit /B

:usage
    call :log "Usage: %~nx0 php_vers php_tag_vers cpp_vers"
exit /B

:MAIN
set pfb_php_vers=%1
set pfb_php_tag_vers=%2
set pfb_cpp_vers=%3

if [%pfb_php_vers%] == [] (
    call :usage
    echo pfb_php_vers varible not set
    exit 1
)

if [%pfb_php_tag_vers%] == [] (
    call :usage
    echo pfb_php_tag_vers varible not set
    exit 1
)

if [%pfb_cpp_vers%] == [] (
    call :usage
    echo pfb_cpp_vers varible not set
    exit 1
)


set pfb_build_root=php%pfb_php_vers%\%pfb_cpp_vers%\

@REM copy fbclient.dll for testing
copy /Y "%PFB_FB64_DIR%\fbclient.dll" "%pfb_build_root%x64\php-src\x64\Release_TS"
copy /Y "%PFB_FB64_DIR%\fbclient.dll" "%pfb_build_root%x64\php-src\x64\Release"
copy /Y "%PFB_FB32_DIR%\fbclient.dll" "%pfb_build_root%x86\php-src\Release_TS"
copy /Y "%PFB_FB32_DIR%\fbclient.dll" "%pfb_build_root%x86\php-src\Release"

(for %%a in (x86 x64) do (
    check out or pull PHP version of interest
    if exist %pfb_build_root%\%%a\php-src\.git\ (
        @REM call :log "Checking out PHP-%pfb_php_vers% %%a"
        @REM git -C %pfb_build_root%\%%a\php-src pull || goto :error
    ) else (
        call :log "Cloning PHP-%pfb_php_tag_vers% %%a"
        call phpsdk-%pfb_cpp_vers%-%%a.bat -t php-firebird-sdk-init.bat %pfb_php_tag_vers% || goto :error
    )

    if %%a EQU x86 ( set pfb_x86=1 ) else ( set pfb_x86=0 )

    (for %%n in (0 1) do (
        set pfb_nts=%%n
        call phpsdk-%pfb_cpp_vers%-%%a.bat -t php-firebird-sdk-build.bat || goto :error
    ))
))

@REM check if FireBird\Connector class exists in newly compiled extension
set check_code="if(!class_exists('FireBird\\Connector'))exit(1);"

set TPATH=%PATH%
set PATH=%PFB_FB64_DIR%;%TPATH%
"%pfb_build_root%x64\php-src\x64\Release_TS\php.exe" -dextension=.\php_firebird.dll -r %check_code% || goto :error
"%pfb_build_root%x64\php-src\x64\Release\php.exe" -dextension=.\php_firebird.dll -r %check_code% || goto :error
set PATH=%PFB_FB32_DIR%;%TPATH%
"%pfb_build_root%x86\php-src\Release_TS\php.exe" -dextension=.\php_firebird.dll -r %check_code% || goto :error
"%pfb_build_root%x86\php-src\Release\php.exe" -dextension=.\php_firebird.dll -r %check_code% || goto :error
set PATH=%TPATH%

call :log "PHP %pfb_php_vers% build OK"

@REM copy compiled extension to target directory
copy %pfb_build_root%x64\php-src\x64\Release_TS\php_firebird.dll %PFB_OUTPUT_DIR%php_firebird-%PFB_VERS%-%pfb_php_vers%-%pfb_cpp_vers%-x86_64.dll>nul
copy %pfb_build_root%x64\php-src\x64\Release\php_firebird.dll %PFB_OUTPUT_DIR%php_firebird-%PFB_VERS%-%pfb_php_vers%-%pfb_cpp_vers%-nts-x86_64.dll>nul
copy %pfb_build_root%x86\php-src\Release_TS\php_firebird.dll %PFB_OUTPUT_DIR%php_firebird-%PFB_VERS%-%pfb_php_vers%-%pfb_cpp_vers%.dll>nul
copy %pfb_build_root%x86\php-src\Release\php_firebird.dll %PFB_OUTPUT_DIR%php_firebird-%PFB_VERS%-%pfb_php_vers%-%pfb_cpp_vers%-nts.dll>nul

exit /B 0

:error
    call :log "PHP %pfb_php_vers% build FAILED"

exit /B 1
