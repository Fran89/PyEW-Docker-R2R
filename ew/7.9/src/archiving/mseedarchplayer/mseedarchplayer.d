#
# miniSEED archive player
#
# An earthworm player specifically designed to play data
# directly from a standard BUD or SeisComP miniSEED archive.
#

# Earthworm Configuration Stuff
 MyModuleId     MOD_MSEEDARCHPLAYER  # Module ID
 OutRing        WAVE_RING            # Ring to send waveforms
 HeartbeatInt   30                   # Heartbeat interval in seconds
 LogFile        1                    # If 0. don't write logfile; if 1. do
                                     # if 2. write module log but not stderr/stdout
 Debug          0
 
 StartTime      20110421034210.00    # Format yyyymmddhhmmss.ff
 Duration       3600                 # Duration in seconds
 
 SendLate       0.0                  # (optional) if present, packets will be
                                     #   timestamped this many seconds before
                                     #   current time;
                                     # if absent, packets will have original time
                                     #   stamps

 InterMessageDelayMillisecs 0        # Number of ms to sleep between inserting tracebufs
                                     # regardless of header time stamp! Fastest possible 
                                     # value is 1. This effectively makes mseedplayer a 
                                     # bulk loader.
                                     # Disabled if this is set to 0 -- then all other delay
                                     # settings apply  
 
 MseedArchiveFolder /MseedArchive
 MseedArchiveFormat 0                # 0: BUD; 1: SeisComP     
 
# List of stations to read data from. Maximum of 200 stations
# Same format as used for ew2mseed but record length and format are ignored! 
 SCNLocSz    CML  EHZ CP NONE 512 STEIM1

