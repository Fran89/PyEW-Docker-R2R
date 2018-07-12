@ echo off

@rem Set environment variables used by earthworm modules at run-time
@rem ---------------------------------------------------------------
set EW_INSTALLATION=INST_UNKNOWN
set EW_HOME=c:\earthworm
set EW_VERSION=earthworm_7.9
set EW_RUN_DIR=%EW_HOME%\run_working
set EW_PARAMS=%EW_RUN_DIR%\params\
set EW_LOG=%EW_RUN_DIR%\log\
set EW_DATA_DIR=%EW_RUN_DIR%\data\
set SYS_NAME=%COMPUTERNAME%
set EW_INST_ID=%EW_INSTALLATION%

@rem Set environment variables for Glass compilation
@rem -------------------------------------
set GLASS_DIR=%EW_HOME%\%EW_VERSION%\src\seismic_processing\glass

@rem --------------------------
set TZ=GMT

@rem Set up Visual C++ compilation environment 
@rem ---------------------
@rem call "c:\Program Files\Microsoft Visual Studio\VC98\bin\Vcvars32.bat"
@rem call "c:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"
@rem call "C:\Program Files\Microsoft Visual Studio 8\VC\bin\vcvars32.bat"
@rem call "C:\Program Files (x86)\Microsoft Visual Studio 8\VC\bin\vcvars32.bat"
@rem call "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" 
@rem call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\vcvars32.bat"
@rem   For build that targets 64-bit platform using a 64-bit development machine:
@rem call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall" amd64
@rem   For build that targets 64-bit platform using a 32-bit development machine:
@rem call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall" x86_amd64
@rem   For build that targets 32-bit platform:
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\vcvars32.bat"

@rem Set up Intel Fortran compilation environment
@rem ---------------------
@rem call "c:\Program Files\Microsoft Visual Studio\DF98\bin\dfvars.bat"
@rem call "c:\Program Files\Intel\Compiler\Fortran\9.0\IA32\Bin\ifortvars.bat"
@rem "c:\Program Files\Intel\Compiler\Fortran\9.1\IA32\Bin\ifortvars.bat"
@rem call "c:\Program Files (x86)\Intel\Compiler\Fortran\9.1\IA32\Bin\ifortvars.bat"
@rem IBM Visual FORTRAN 11.1 Professional
@rem "c:\Program Files\Intel\Compiler\11.1\060\bin\ifortvars.bat" ia32 vs2008
@rem Intel Composer XE for Windows
@rem call "C:\Program Files (x86)\Intel\Composer XE 2013\bin\ifortvars.bat" ia32 vs2012shell
@rem Intel Parallel Studio XE 2016 for Windows (ia32 for 32-bit host and target,
@rem   intel64 for 64-bit host and target, ia32_intel64 for 32-bit host and 64-bit target)
call "C:\Program Files (x86)\IntelSWTools\compilers_and_libraries\windows\bin\ifortvars.bat" ia32

@rem Set the path
@rem ------------------------------------------------------
set PATH=%EW_HOME%\%EW_VERSION%\bin;%PATH%

@rem Set the include path
@rem -------------------------------------------------------------------------
@rem Include "include_win" directory for "NtWin32.Mak" file
set INCLUDE=%EW_HOME%\%EW_VERSION%\include;%EW_HOME%\%EW_VERSION%\include_win;%INCLUDE%
@rem Set the include MySQL path
@rem set INCLUDE=%EW_HOME%\%EW_VERSION%\src\archiving\mole\mysql-connector-c-6.0.2\include;%INCLUDE%
@rem If you want to compile ew2moledb with another MySQL library, then
@rem Set the INCLUDE variable with your own mysql include dir and
@rem copy the library file mysqlclient.lib to the directory
@rem %EW_HOME%\%EW_VERSION%\src\archiving\mole\mysql-connector-c-build\lib

@rem Set the library path
@rem -----------------------------------------------------------------
set WIN_FLUSH_OBJ=commode.obj
set LIB=%EW_HOME%\%EW_VERSION%\lib;%LIB%

@rem -----------------------------------------------------------------
@rem Set extra flags you want for the C compiler here
@rem The /Od flag configures build for no optimization
@rem   Remove /Od for default optimization; add /Ox for maximum optimization
@rem set GLOBALFLAGS=/D_WINNT /D_INTEL /Od
@rem   Don't define _USE_32BIT_TIME_T for build that targets 64-bit platform (not supported):
@rem set GLOBALFLAGS=/D_WINNT /D_INTEL /D_CRT_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS /Od
@rem   Define _CRT_SECURE_NO_DEPRECATE and _CRT_SECURE_NO_WARNINGS to suppress deprecation warnings
@rem   Define _USE_32BIT_TIME_T for 32-bit-sized 'time_t' type (on 32-bit-platform builds):
set GLOBALFLAGS=/DWIN32_LEAN_AND_MEAN /D_WINNT /D_INTEL /D_CRT_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS /D_USE_32BIT_TIME_T /Od

@rem Set INIT, the path to the tools.ini file which gets used by
@rem nmake to set global compiler switches
@rem -----------------------------------------------------------------
set INIT=%EW_HOME%\%EW_VERSION%\environment
