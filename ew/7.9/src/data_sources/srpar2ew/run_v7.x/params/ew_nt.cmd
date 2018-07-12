@ echo off

@rem Set environment variables used by earthworm modules at run-time
@rem ---------------------------------------------------------------
set EW_INSTALLATION=INST_UNKNOWN
set EW_HOME=c:\earthworm
set EW_VERSION=v7.4
set EW_PARAMS=%EW_HOME%\%EW_VERSION%\run_symres\params
set EW_LOG=%EW_HOME%\%EW_VERSION%\run_symres\log\
set EW_DATA=%EW_HOME%\%EW_VERSION%\run_symres\data\
set SYS_NAME=%COMPUTERNAME%

@rem Set environment variables for Oracle
@rem -------------------------------------
set SCHEMA_DIR=%EW_HOME%\%EW_VERSION%\src\oracle\schema
set APPS_DIR=%EW_HOME%\%EW_VERSION%\src\oracle\apps

@rem Set environment variables for Glass compilation
@rem -------------------------------------
set GLASS_DIR=%EW_HOME%\%EW_VERSION%\src\seismic_processing\glass

@rem Set environment variables for the Web server
@rem ---------------------------------------------
set WEB_DIR=%EW_HOME%\web

@rem --------------------------
set TZ=GMT

@rem Set up Visual C++ compilation environment
@rem ---------------------
@rem call "c:\Program Files\Microsoft Visual Studio\VC98\bin\Vcvars32.bat"
@rem call "c:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"
call "c:\Program Files\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"

@rem Set up Intel Fortran compilation environment
@rem ---------------------
@rem call "c:\Program Files\Microsoft Visual Studio\DF98\bin\dfvars.bat"
@rem call "c:\Program Files\Intel\Compiler\Fortran\9.0\IA32\Bin\ifortvars.bat"
@echo skipping call "c:\Program Files\Intel\Compiler\Fortran\9.1\IA32\Bin\ifortvars.bat"

@rem Set the path
@rem ------------------------------------------------------
set Path=%Path%;c:\home\bin;c:\local\bin;%EW_HOME%\%EW_VERSION%\bin

@rem Set the include path
@rem -------------------------------------------------------------------------
set INCLUDE=%INCLUDE%;%EW_HOME%\%EW_VERSION%\include;%SCHEMA_DIR%\src\include;%SCHEMA_DIR%\src\include\internal;%APPS_DIR%\src\include

@rem Set the library path
@rem -----------------------------------------------------------------
set LIB=%LIB%;%EW_HOME%\%EW_VERSION%\lib

@rem -----------------------------------------------------------------
@rem Set extra flags you want for the C compiler here
@rem set GLOBALFLAGS=/D_WINNT /D_INTEL
@rem you might want to set this NO_DEPRECATE if using Visual Studio 2005
set GLOBALFLAGS=/D_WINNT /D_INTEL /D_CRT_SECURE_NO_DEPRECATE /D_USE_32BIT_TIME_T


@rem  ORACLE CONFIGURATION
@rem -------------------------------------------------------------------------
set ORACLE_HOME=c:\orant
set INCLUDE=%INCLUDE%;%ORACLE_HOME%\oci80\include
set LIB=%LIB%;%ORACLE_HOME%\oci80\lib\msvc
@rem Needed for properly getting the oracle oci library directory on NT
set ORACLE_OCI_VER=80

@rem Set INIT, the path to the tools.ini file which gets used by
@rem nmake to set global compiler switches
@rem -----------------------------------------------------------------
set INIT=%EW_HOME%\%EW_VERSION%\environment
