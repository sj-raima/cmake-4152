@echo off

rmdir /s /q HELLO_WORLDODBC.rdm 2> nul
rmdir /s /q sysstats.rdm 2> nul

echo. | odbc01Example.exe > std.out
set stat=%errorlevel%

if %stat% neq 0 (
   echo Error in running Hello World ODBC Tutorial Test
)

type std.out | FIND /C "Hello World!" > counter
set /p val=<counter
if /I not %val% == 1 (
    echo Error in finding expected value "Hello World!" counts equal %val% instead of 1.
    type std.out
    set stat=1
)

del counter
rmdir /s /q HELLO_WORLDODBC.rdm
rmdir /s /q sysstats.rdm
del std.out
del tfserver.lock >nul

exit %stat%
