@echo off
@rem  RTPD Log File Management
@rem  Make daily logs out of the ever-growing rtpd.log
@rem
@rem ----------------------------------------------------
@rem  Schedule this script to run daily at 23:58 to rename
@rem  the existing rtpd.log file for today.  RTPD will 
@rem  automatically generate a new rtpd.log file.
@rem 
@rem  To run this script automatically on NT:
@rem   1. turn on "Schedule" service (make it automatic)
@rem   2. use "at" command to enter the script into the
@rem      scheduler (See NT help for details).  Example:
@rem      at 00:00 /every:M cmd /c c:\fullpath\to\your.cmd
@rem
@rem  On Win2000 schedule this script in:
@rem    Start-> Control Panel -> Scheduled Tasks
@rem ----------------------------------------------------

cd c:\reftek\log

c:\local\bin\date.exe "+rename rtpd.log  rtpd_20%%y%%m%%d.log"  > tmprename.cmd

call tmprename.cmd
