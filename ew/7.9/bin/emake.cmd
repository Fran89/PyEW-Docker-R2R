@ echo off
if "%OS%" == "Windows_NT" goto _WINNT_make
goto _OS2_make

:_WINNT_make
nmake /f Makefile.nt %1
goto end

:_OS2_make
nmake /f Makefile.os2 %1

:end
