@echo off
rem # nt_copy_exe.bat                                    2003/11/26:LDD
rem # Used to copy executable from source directory to version\bin directory 

setlocal
echo ----------------
@echo on
cd %1
copy /y %1.exe %EW_HOME%\%EW_VERSION%\bin
@echo off
endlocal