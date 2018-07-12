#  configuration file for naqs2ew

MyModuleId        MOD_NAQS2EW
RingName          WAVE_RING
HeartBeatInt      30
LogFile           1
Debug             1    # 0=no debugging messages
                       # 1=log a few header-type lines per packet.
                       # 2=log header lines plus all data samples per packet.
SocketDebug       0    # 0=off, non-zero=log debug messages (VERBOSE!!!!)

DataServer        130.118.43.26 28002 # IP address and Port of NaqsServer
DSuser            ppicker geteqsnow
