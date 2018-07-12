@echo off
set INCLUDE=%INCLUDE%;%EW_HOME%\%EW_VERSION%\include_cpp
set INCLUDE=%INCLUDE%;%EW_HOME%\%EW_VERSION%\src\libsrc_cpp\servers
set INCLUDE=%INCLUDE%;%EW_HOME%\%EW_VERSION%\src\libsrc_cpp\util
set INCLUDE=%INCLUDE%;%EW_HOME%\%EW_VERSION%\src\libsrc_cpp\util\sockets
echo C++ environment variable set
echo on