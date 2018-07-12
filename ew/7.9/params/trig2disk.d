# trig2disk:
# I listen to trigger messages (TYPE_TRIGLIST_SCNL), 
# gather the listed waveform snippets from a list of WaveServerV's,
# and write them to disk file(s) in the requested format.
#------------------------------------------------------------------
 MyModuleId      MOD_TRIG2DISK
 RingName        HYPO_RING
 HeartBeatInt    30

 LogFile         1            # 0 => don't create a disk log file.
                              # 1 => write a disk log file.
                              # 2 => log to disk file but not stderr/stdout

#Debug                        # Debug info will be logged if uncommented

# What message logos to listen to. Can be more than one.
# The type is hard coded to TYPE_TRIGLIST_SCNL
#-------------------------------------------------------
 GetEventsFrom   ${EW_INST_ID}  MOD_WILDCARD


# IP addresses/Port of the WaveServers
#------------------------------------------
 WaveServer      127.0.0.1       16022   # talk to local waveserver	
 WaveServer      111.222.333.444 16031	 # talk to other waveserver

 TimeoutSeconds  30       # If a WaveServer doesn't talk to us in this
                          # many seconds, we'll abort that request

 MaxTraces       256	  # Max number of traces we'll ever see in one event

 TraceBufferLen  256  	  # kilobytes of the largest trace snippet 
                          # trig2disk will ever have to deal with.

# Optional NEW COMMAND in EW v7.3 Postproc allows running of a program or script that will process new files
# tow arguments are sent to the script : date (YYYYMMDD) and time (HHMM) of the processed event.
# Command sent : "myscript EventData EventTime".
# Otherwise, it should know where to run and what to do with new files:
#
#Postproc  myscript  # the myscript or program should be in the params directory or an absolute path must be provided.


# SCNL list of stations to write for each trigger message.  
# Any SCNLs listed here will be gathered in addition
# to the SCNLs in the trigger message.
# written in addition to SCNLs in the trigger message.
#--------------------------------------------------------
#TrigStation  IRGCH ATT NC --

 MinDuration  30          # Minimum number of seconds to save for extra
                          # channels that we are saving (i.e., those chans
                          # not in the TYPE_TRIGLIST message)


# Output control
#---------------
 GapThresh    1000        # number of sample periods after which we
                          # declare a gap. For some output formats,
                          # a gap means a new output file.

 OutDir       d:\events   # base directory for output data files

 DataFormat   tank        # Possible DataFormats are:
                          #   suds
                          #   sac
                          #   ah 
                          #   mseed 
                          #   seisan
                          #   gse_int  (GSE)
                          #   uw  (UW2 format = University of Washington) - Unix only
                          #   wfdisc  (CSS table and .w files) - Unix only
                          #   tank     (tankplayer input/raw Earthworm)

 OutputFormat intel       # Possible OutputFormats are: 
                          #   intel
                          #   sparc
                          # Files will be written out in the specified byte 
                          # order. Make sure it's appropriate for platform 
                          # where the output files will be used.
 
# Optional queue commands
#------------------------
#QueueSize 5              # Optional: # trigger msgs to hold in queue; 
                          # default is 10

 QueueFile trig2disk.que  # Optional: queue dumpfile name, for saving state of queue


#DelayTime 5              # Optional: trig2disk will wait this many seconds
                          #  from the time it receives a trigger message 
                          #  until it starts to process the message
                          #  Allows trig2disk to wait for late-arriving wf.

# THE END
