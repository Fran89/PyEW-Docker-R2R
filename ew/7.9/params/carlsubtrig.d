#
#	IMPORTANT NOTE ABOUT RUNNING CARLSUBTRIG
#
# Carlsubtrig supports the ability to use late arriving station triggers in the
# subnet trigger logic. It maintains an internal clock which is set a fixed
# number of seconds (the latency period) behind wall-clock (real, system)
# time. This latency period allows for delayed delivery of station trigger
# messages. Carlsubtrig uses this internal clock to compare to station
# trigger times. Once the station trigger-on or -off time is later than the
# internal clock time, this trigger status change is noticed by carlsubtrig. In
# order for times to be compared between machines, all machines should
# be synchronized within a few seconds or better. Xntp (available with
# Solaris2.6 and also public domain) is a good choice for time
# synchronization. 

# Because Carlsubtrig uses both system and data time to perform the
# subnet trigger logic, the system time must be set to UTC. If this is
# not done, no triggers will occur.
# Under NT, TZ must be set to GMT in ew_nt.cmd in order to run under
# GMT time.
#

#
# CarlSubTrig's Parameter File
#

#
#  Basic Earthworm Setup
#
MyModuleId	MOD_CSU_TEST	# Module id for this instance of CarlSubTrig -
				#
Debug		2		# Write out debug messages? (0 = No,
				#   1 = Minimal, 3 = Chatterbox )
RingNameIn	PICK_RING	# Name of ring from which station triggers 
				#   will be read - REQUIRED.
RingNameOut	HYPO_RING	# Name of ring to which triggers will be
				#   written - REQUIRED.
HeartBeatInterval	30	# Heartbeat Interval (seconds).


# PlaybackMode - a new option to allow playback of historic data, start time of clock 
# 		comes from the first trigger time (not the system).
#PlaybackMode 1

#
# CarlSubTrig Specific Setup
#
StationFile	"pnsn_trig.sta"	# Name of file containing station information -
				#   REQUIRED.
Latency		15		# Number of seconds that the Network clock
				#   is behind wall clock REQUIRED.
NetTriggerDur   10		# Number of seconds for the base network
				#   trigger duration REQUIRED.
SubnetContrib   15		# Addition contribution to network trigger
				#   duration for each subnet that triggered
                                #   REQUIRED.
PreEventTime	10		# Number of seconds added to beginning of
				#   network trigger REQUIRED.
MaxDuration     500		# Maximum duration allowed for network trigger
DefStationDur   120		# Default number of seconds for station
				#   trigger duration if the trigger-off 
				#   message is not received. REQUIRED.
ListSubnets     1               # Flag to list untriggered stations
                                #   =0 or command not present: list all 
                                #        triggered stations 
                                #   =1 list all stations in triggered subnets
                                #   =2 list all stations in triggered subnets 
                                #        plus any other triggered stations. 
                                #   =3 list all stations in subnets that had
                                #        any stations triggered
AllSubnets      10              # If this many subnets trigger, put wildcard
                                #   SCN in event message
CompAsWild                      # Flag (no value) to list component names in
                                #   trigger messages as `*' (wildcard).
MaxTrigMsgLen   30000           # maximum length (bytes) of a triglist message;
                                #   up to MAX_BYTES_PER_EW (earthworm.h).

# IgnoreCoincidentCheck 3	# if more than this number of subnets triggers, ignore the coincident check
# CoincidentStaTriggers 6       # if more than this number of station triggers align exactly on the second, reject the Subtrig message
				# this OPTIONAL parameter is off if not entered or set to 0. Used for getting rid of telemetry glitches
				# new in carlsubtrig v1.0.3 (EW 7.5)
				
EarlyWarning            1	# 1 sets early warning mode. In this mode, carlsubtrig
				# launches an initial subnetwork trigger as soon as
				# the network trigger is set ON and then launches the
				# main network trigger as normal. Both messages have the
				# same event id.

# Load the next valid trigger sequence number
@trig_id.d            # this name is hard-coded; do not change (SEE BELOW)
#
# NEW in earthworm 7.4, you can ignore above and specify a trig_id.d file (that must contain next_id in it) using
# the following directive:
#
# TrigIdFilename trig/trig_id.d    # this allows you to put the trig_id.d file somewhere else or have more than one carlsubtrig running!
#

# List the message logos to grab from transport ring
#              Installation       Module          Message Types (hard-wired)
GetEventsFrom  INST_WILDCARD    MOD_WILDCARD    # TYPE_CARLSTATRIG # REQUIRED.

# Non-seismic or other channels that should be included in all event messages
# List one SCN per line, as many as you need
Channel  *.TIM.UW 
Channel  LON.LHZ.UW

# Subnet definitions for the CarlSubTrig Earthworm module
# Each Subnet must be on a single line
# Subnet  Minimum to      List of Station.Component.Network.Location
# Number  Trigger         Codes (space delimited)
# ------- ---  -------------------------------------------
Subnet 0   3   CRF.EHZ.UW.-- EPH.EHZ.UW.-- GBL.EHZ.UW.-- MDW.EHZ.UW.-- OT3.EHZ.UW.-- RC1.EHZ.UW.-- VT2.EHZ.UW.-- WA2.EHZ.UW.-- WRD.EHZ.UW.--
Subnet 1   4   BRV.EHZ.UW.-- ET3.EHZ.UW.-- GBL.EHZ.UW.-- MDW.EHZ.UW.-- OT3.EHZ.UW.-- PRO.EHZ.UW.-- RSW.EHZ.UW.-- WA2.EHZ.UW.-- WIW.EHZ.UW.-- MJ2.EHZ.UW.-- WG4.EHZ.UW.--
Subnet 2   4   ET3.EHZ.UW.-- JBO.EHZ.UW.-- LNO.EHZ.UW.-- PAT.EHZ.UW.-- PRO.EHZ.UW.-- RSW.EHZ.UW.-- WIW.EHZ.UW.-- WG4.EHZ.UW.--
Subnet 3   3   CBS.EHZ.UW.-- DY2.EHZ.UW.-- EPH.EHZ.UW.-- ETW.EHZ.UW.-- NEL.EHZ.UW.-- SAW.EHZ.UW.-- TBM.EHZ.UW.-- WAT.EHZ.UW.-- 
Subnet 4   3   DPW.EHZ.UW.-- DY2.EHZ.UW.-- EPH.EHZ.UW.-- OD2.EHZ.UW.-- OT3.EHZ.UW.-- RC1.EHZ.UW.-- SAW.EHZ.UW.-- WRD.EHZ.UW.--
Subnet 5   4   ASR.EHZ.UW.-- BRV.EHZ.UW.-- ELL.EHZ.UW.-- FMW.EHZ.UW.-- GL2.EHZ.UW.-- GLK.EHZ.UW.-- LO2.EHZ.UW.-- MOX.EHZ.UW.-- NAC.EHZ.UW.-- RSW.EHZ.UW.-- TBM.EHZ.UW.-- TWW.EHZ.UW.-- VT2.EHZ.UW.-- WPW.EHZ.UW.-- YA2.EHZ.UW.-- 
Subnet 6   3   BHW.EHZ.UW.-- CMW.EHZ.UW.-- ETW.EHZ.UW.-- HTW.EHZ.UW.-- JCW.EHZ.UW.-- JCW.EHZ.UW.-- MBW.EHZ.UW.-- NEL.EHZ.UW.-- RMW.EHZ.UW.-- RPW.EHZ.UW.-- TWW.EHZ.UW.-- 
Subnet 7   3   BHW.EHZ.UW.-- BLN.EHZ.UW.-- CMW.EHZ.UW.-- GMW.EHZ.UW.-- HDW.EHZ.UW.-- HTW.EHZ.UW.-- JCW.EHZ.UW.-- JCW.EHZ.UW.-- MBW.EHZ.UW.-- MCW.EHZ.UW.-- OHW.EHZ.UW.-- PGW.EHZ.UW.-- RPW.EHZ.UW.-- STW.EHZ.UW.-- 
Subnet 8   4   BHW.EHZ.UW.-- BLN.EHZ.UW.-- FMW.EHZ.UW.-- GHW.EHZ.UW.-- GMW.EHZ.UW.-- GSM.EHZ.UW.-- HTW.EHZ.UW.-- RMW.EHZ.UW.-- SMW.EHZ.UW.-- TWW.EHZ.UW.--
Subnet 9   4   LCW.EHZ.UW.-- BOW.EHZ.UW.-- CPW.EHZ.UW.-- FMW.EHZ.UW.-- GHW.EHZ.UW.-- GSM.EHZ.UW.-- KOS.EHZ.UW.-- LMW.EHZ.UW.-- MEW.EHZ.UW.-- RMW.EHZ.UW.-- RVC.EHZ.UW.-- SMW.EHZ.UW.--
Subnet 10  4   CDF.EHZ.UW.-- EDM.EHZ.UW.-- ELK.EHZ.UW.-- FL2.EHZ.UW.-- HSR.EHZ.UW.-- JUN.EHZ.UW.-- KOS.EHZ.UW.-- LVP.EHZ.UW.-- MTM.EHZ.UW.-- SEP.EHZ.UW.-- SHW.EHZ.UW.-- SOS.EHZ.UW.-- STD.EHZ.UW.-- TDL.EHZ.UW.-- YEL.EHZ.UW.-- 
Subnet 11  3   AUG.EHZ.UW.-- GUL.EHZ.UW.-- PGO.EHZ.UW.-- TDH.EHZ.UW.-- VBE.EHZ.UW.-- VFP.EHZ.UW.-- VG2.EHZ.UW.-- VGB.EHZ.UW.-- VLL.EHZ.UW.-- VLM.EHZ.UW.-- 
Subnet 12  3   BLN.EHZ.UW.-- GMW.EHZ.UW.-- HDW.EHZ.UW.-- OBC.EHZ.UW.-- OFR.EHZ.UW.-- OSR.EHZ.UW.-- OOW.EHZ.UW.-- OSD.EHZ.UW.-- OCP.EHZ.UW.-- OTR.EHZ.UW.-- SMW.EHZ.UW.-- STW.EHZ.UW.--
Subnet 13  3   ASR.EHZ.UW.-- AUG.EHZ.UW.-- CDF.EHZ.UW.-- ELL.EHZ.UW.-- GL2.EHZ.UW.-- GLK.EHZ.UW.-- GUL.EHZ.UW.-- MOX.EHZ.UW.-- MTM.EHZ.UW.-- NAC.EHZ.UW.-- WPW.EHZ.UW.-- YA2.EHZ.UW.-- TRW.EHZ.UW.-- 
Subnet 14  4   TKO.EHZ.UW.-- KMO.EHZ.UW.-- LVP.EHZ.UW.-- NLO.EHZ.UW.-- PGO.EHZ.UW.-- RVW.EHZ.UW.-- TDH.EHZ.UW.-- VBE.EHZ.UW.-- VG2.EHZ.UW.-- VLM.EHZ.UW.-- WPO.EHZ.UW.-- SSO.EHZ.UW.-- MPO.EHZ.UW.-- 
Subnet 15  4   LCW.EHZ.UW.-- BOW.EHZ.UW.-- CPW.EHZ.UW.-- OBH.EHZ.UW.-- OSR.EHZ.UW.-- ONR.EHZ.UW.-- OOW.EHZ.UW.-- OSD.EHZ.UW.-- SMW.EHZ.UW.--
Subnet 16  4   ASR.EHZ.UW.-- CDF.EHZ.UW.-- FMW.EHZ.UW.-- GLK.EHZ.UW.-- KOS.EHZ.UW.-- LMW.EHZ.UW.-- LO2.EHZ.UW.-- NAC.EHZ.UW.-- RCS.EHZ.UW.-- RER.EHZ.UW.-- RVC.EHZ.UW.-- TWW.EHZ.UW.-- WPW.EHZ.UW.--
Subnet 17  3   GL2.EHZ.UW.-- JBO.EHZ.UW.-- PAT.EHZ.UW.-- VBE.EHZ.UW.-- VCR.EHZ.UW.-- VFP.EHZ.UW.-- VGB.EHZ.UW.-- VIP.EHZ.UW.-- VTH.EHZ.UW.-- 
Subnet 18  3   BOW.EHZ.UW.-- CPW.EHZ.UW.-- KMO.EHZ.UW.-- NLO.EHZ.UW.-- ONR.EHZ.UW.-- RVW.EHZ.UW.-- TKO.EHZ.UW.-- WIB.EHZ.UW.--
Subnet 19  3   CDF.EHZ.UW.-- JUN.EHZ.UW.-- SHW.EHZ.UW.-- SOS.EHZ.UW.-- STD.EHZ.UW.-- YEL.EHZ.UW.-- 
Subnet 20  3   BPO.EHZ.UW.-- GMO.EHZ.UW.-- NCO.EHZ.UW.-- TCO.EHZ.UW.-- VBE.EHZ.UW.-- VCR.EHZ.UW.-- VG2.EHZ.UW.-- VIP.EHZ.UW.-- SSO.EHZ.UW.-- FBO.EHZ.UW.-- HBO.EHZ.UW.-- VRC.EHZ.UW.-- VSP.EHZ.UW.-- HAM.EHZ.UW.-- LAB.EHZ.UW.--
Subnet 21  4   LO2.EHZ.UW.-- FMW.EHZ.UW.-- RER.EHZ.UW.-- RCM.EHZ.UW.-- RCS.EHZ.UW.--
Subnet 22  3   TCO.EHZ.UW.-- BPO.EHZ.UW.-- FBO.EHZ.UW.-- HBO.EHZ.UW.-- HSO.EHZ.UW.-- DBO.EHZ.UW.-- BBO.EHZ.UW.-- VRC.EHZ.UW.-- VSP.EHZ.UW.-- HAM.EHZ.UW.-- LAB.EHZ.UW.-- HOG.EHZ.UW.--
Subnet 23  4   KMO.EHZ.UW.-- TKO.EHZ.UW.-- WPO.EHZ.UW.-- MPO.EHZ.UW.-- SSO.EHZ.UW.-- FBO.EHZ.UW.-- RNO.EHZ.UW.-- HSO.EHZ.UW.-- DBO.EHZ.UW.-- 
Subnet 24  4   ASR.EHZ.UW.-- ELL.EHZ.UW.-- FMW.EHZ.UW.-- GSM.EHZ.UW.-- NAC.EHZ.UW.-- RMW.EHZ.UW.-- RVN.EHZ.UW.-- TBM.EHZ.UW.-- TWW.EHZ.UW.-- WPW.EHZ.UW.--
