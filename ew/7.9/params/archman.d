# Config file for archman

# Normal Earthworm stuff - all these are required
MyModuleId		MOD_ARCHMAN
InRingName		WAVE_RING
OutRingName		WAVE_RING
HeartBeatInterval	30
LogFile			1

# file where state info. is stored between invocations
ArchmanStore	archman.dat

# Uncomment the next line for debugging info
Verbose

# The size of archived data semgents
ContinuousDataSegmentSize	1 hour

# Delay between first data message being recorded in Earthworm and all data being
# archived (to allow time for slower data to be recorded)
ContinuousDataDelay		20 minutes

# Whether to call waveman2disk every time new data is seen, thus continuously
# updating output files. Comment out for original archman behaviour
ContinuousOutput

# List of stations (in s c n l format) to archive - there can be
# as many of these as you need
SaveSCNL             EDI  HHZ GB 00     # reftek
SaveSCNL             EDI  HHN GB 00     # reftek
SaveSCNL             EDI  HHE GB 00     # reftek
SaveSCNL             ESK  HHZ GB 00     # reftek
SaveSCNL             ESK  HHN GB 00     # reftek
SaveSCNL             ESK  HHE GB 00     # reftek
SaveSCNL             HTL  HHZ GB 00     # sdas
SaveSCNL             HTL  HHN GB 00     # sdas
SaveSCNL             HTL  HHE GB 00     # sdas
SaveSCNL             EAU  SHZ GB 00     # scream
SaveSCNL             EBL  SHZ GB 00     # scream
SaveSCNL             ESY  SHZ GB 00     # scream
SaveSCNL             EAB  SHZ GB 00     # scream
SaveSCNL             EBH  SHZ GB 00     # scream
SaveSCNL             EDU  SHZ GB 00     # scream
SaveSCNL             VELA SHZ GB 00     # scream
SaveSCNL             ESK  B1Z GB 00     # naqs
SaveSCNL             ESK  B1N GB 00     # naqs
SaveSCNL             ESK  B1E GB 00     # naqs
SaveSCNL             ESK  B2Z GB 00     # naqs
SaveSCNL             ESK  B2N GB 00     # naqs
SaveSCNL             ESK  B2E GB 00     # naqs
SaveSCNL             H09N EHZ GB 00     # cd10
SaveSCNL             H09N EHN GB 00     # cd10
SaveSCNL             H09N EHE GB 00     # cd10
SaveSCNL             H09W EHZ GB 00     # cd10
SaveSCNL             H09W EHN GB 00     # cd10
SaveSCNL             H09W EHE GB 00     # cd10

# List of waveservers from which data will be obtained - there can be
# as many of these as you need
WaveServer	192.171.143.25		16022

# A bunch of configuration parameters passed through to waveman2disk -
# see waveman2disk.d for details
TimeoutSeconds		500
MaxTraces		100
TraceBufferLen		8000
GapThresh		20
MinDuration		30


# the output format details for waveman2disk -
# see waveman2disk.d for details
DataFormat		seisan
OutDir			/users/earthwor/data/ew-develop-mhlc-7.0/archive
OutputFormat		intel

# List of directory pairs - any files in the first (source) directory
# will be copied to the second (destination) directory, then deleted.
# This allows data to be copied from one computer to another over
# an NFS (or similar) disk mount
#MoveDir /users/earthwor/data/archive /users/earthwor/linux/archive
#MoveDir /users/earthwor/data/trigger /users/earthwor/linux/trigger

# StartTime sets the time to start if there is no .dat file.  This 
# is optional, if it is not set, archman will start with the begining 
# tank from the waveserver
#StartTime "2009/05/27 16:20:00"
