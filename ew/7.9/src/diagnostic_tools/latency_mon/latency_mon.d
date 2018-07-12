
#
#                     Latency_mon's Configuration File
#
MyModId    MOD_LATENCY_MON     # This instance of latency_mon
InRing           WAVE_RING     # Input waveform ring
HeartbeatInt            30     # Heartbeat interval, in seconds

# ATWC-style station files. Specify these, OR the Findwave file below.
StaFile        "pick_wcatwc.sta" # File containing stations to be displayed
StaDataFile    "station.dat"   # File with information about stations

# FindWave-style, debug-mode station list. Specify this OR the ATWC files above
FindWaveFile	"zorch.txt"

# FileRowSize -- number of entries per circular station file
#                1000 rows apx 24K bytes
#
FileRowSize 1000

#
LogPath "c:\earthworm\run\log\status\" # Path where to put latency information
NumTracePerScreen       40     # Number of stations to show on visible screen
                               # (the rest will scroll)

PrinterPath "\\Earlybird3\lq870" # Printer path-PRN, COMx, or \\cpu\printername
#
Debug                    0     # If 1, print debugging message
