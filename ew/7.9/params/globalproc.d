
#              This is globalproc's Parameter File

MyModuleId  MOD_GLOBALPROC # Module id for this instance of eqproc
InRingName  COINC_RING      # Ring to get input from
OutRingName	COINC_RING	   # Ring to put locations into
HeartBeatInterval 30
LogFile     1              # 0 to turn off disk log file; 1 to turn on disk log
                           # 2 to write module log but not to stderr/stdout

# Reporting strategy: We accept a table of reporting time intervals in
# seconds since receipt of initial quake message from glass. 
# The second column specifies whether event is reported only if it had
# changed since the last report.  If set to 1, events will be reported
# irrespective of changes.  Otherwise, only changed events are reportred. 
#

Report 		0		1
Report 		20		0
Report 		30		0
Report 		40		0
Report 		120		1

MinPhases  10

# The message logos to grab from transport ring
#              Installation       Module          Message Types
GetPicksFrom   INST_WILDCARD    MOD_WILDCARD    # TYPE_PICK_RB  & TYPE_PICK2K & TYPE_CODA2K
GetAssocFrom   INST_WILDCARD    MOD_WILDCARD	# TYPE_QUAKE2K & TYPE_LINK

Debug 2	# can be 0, 1, or 2

