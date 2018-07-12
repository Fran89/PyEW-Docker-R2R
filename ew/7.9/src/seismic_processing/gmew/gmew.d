# Configuration file for gmew
#
# Required Commands: 
# The following five commands are always required by gmew.
# Other commands below may be required when certain options are given.
#
# Earthworm Transport commands
# RingInName <ring>: specify the name of the earthworm transport ring from
#  which to read TYPE_HYP2000ARC messages for event notification.

RingInName HYPO_RING

# RingOutName <ring>: specify the name of the earthworm transport ring from
#  which to write Magnitude, Heartbeat and Error messages. This may be
#  the same is the RingInName if desired.

RingOutName HYPO_RING

# MyModId <module-id>: specify the module ID for gmew
 MyModId MOD_GMEW

# HeartBeatInterval int: How often gmew should beat its heart.

HeartBeatInterval 30

# getEventsFrom  INST_ID MOD_ID
# Specify the installation ID and module ID names from which to get
# hypoinverse archive messages to read event data. These names may be
# selected from earthworm.d and earthworm_global.d. No default values.
# Use as many of these commands as necessary.

getEventsFrom INST_WILDCARD MOD_WILDCARD


# maxSta number
# The number of stations to be used by gmew must be specified with
# the maxSta command. This is the number of unique SNs to be used; each
# SN combination can have any number of components. The maxSta command
# must come before any "Pri" commands.

maxSta 100

# maxDist km
# The maximum epicenter - station distance in kilometers. 
# Stations lying outside this radius will not be used by gmew.
#
maxDist 600


# maxTrace nsamples
# The maximum number of trace data samples to process from one SCN.
# This is the product of the trace length in seconds and the highest 
# expected sample rate. For example, 10 minutes of data at 100 samples per
# second gives 60000 samples.

maxTrace 60000


# staLoc: station location source
#    Choices: File <loc-file> - get station locations from a hyp2000-format
#                               file named loc-file
#             EWDB - query the EW database for station locations; access info
#                    given below.

staLoc File /earthworm/run/params/hypoinverse/stas/uw_sta.hinv


# Regional Velocity model: must be specified as "@vel_model_file". Typically
#    this will be the same file as used for binder_ew.

@uw_velocity.d


# traceSource: Where localmag will obtain trace data
#    choices are: waveServer <server-list> - raw traces from wave_servers; 
#                              server-list is either a space separated list
#                              of ip-address:port or 
#                           "File <server-file>"
#    default: waveServer with list of servers read from $EW_PARAMS/servers

traceSource waveServer 123.45.67.89:16021 123.45.67.89:16022

# wsTimeout: wave_server timeout in milliseconds
#    default: 5000 milliseconds

# wsTimeout 10000

# the number of seconds to wait before hitting the waveServer to get waveforms, 
# useful in RT mode
extraDelay 30

# Add STA COMP NET
# Del STA COMP NET
# SCN selection is done by a selection list and a rejection list.
# The station selection command 'Add' specifies the names of 
# STA (station), COMP (component), and NET (network). 
# The wildcard '*' may be used for any or all of STA, COMP, NET.
# COMP may be given as the first two letters of the component name,
# to select all direction desgnators are accepted for that component.
# If no Add commands are given, all SCNs will be selected.
# The Del command is used to put SCNs on the rejection list:
# for example: "Add * EL UW" followed by "Del LAB EL UW" to include
# all ELE and ELN components from UW except LAB.ELE.UW and LAB.ELN.UW.
# List as many Add and Del commands as desired, one to a line

Add * BH *
Add * HH *
Add * SL *
Add * EL *
Del LAB * UW


# respSource: Tells gmew where to get response information; 
#   may be ommitted if traceSource points to Wood-Anderson 
#    Choices: EWDB - query the EW databse for pole-zero-gain responses.
#             SAC <pz-filename-format> - find response files in the 
#                            SAC directory given with SACsource.
#                            filename-format is a string similar to printf
#                            format: %S, %C, %N are replaced by station, 
#                            component and network names, respectively,
#                            all in upper case. Similarly %s, %c, %n work
#                            for lower-case station, component and network.
#                            The digraph %% stands for %; all other 
#                            characters are taken literally.
#                            The response file format is that used by SAC.
#             File <dirname> <pz-filename-format> - find response files in
#                            directory dirname with file names given by
#                            pz-filename-format, as above. dirname can be 
#                            a full directory name, or relative to $EW_PARAMS.
# 
#    default: none; you must specify one of the above choices.

respSource File responses %S_%C_%N.pz

# ResponseInMeters - an optional flag to convert SAC P&Z from meters into what is required 
#                       units of nanometers. Allows use of rdseed -pf  files directly.


# traceTimes start end
# The start and end times for traces is set here. 
#   <start> is the number of seconds before estimated P arrival to start
#           the trace.
#   <end> is the number of seconds after estimated S arrival to end the trace.
#   default: traceTimes 5.0 60.0 # starts trace 5 seconds before estimated P
#           arrival and ends trace 60 seconds after estimated S arrival.

traceTimes 5.0 60.0


# searchWindow A B C D
# The time window for searching for peak amplitudes is set here.
#    <A> is the multiple of the estimated interval P - S to start the 
#        search before estimated S arrival.
#    <B> is the minimum number of seconds before estimated S arrival to
#        start the peak search.
#    <C> is the multiple of the estimated interval P - S to end the
#        search after the estimated S arrival.
#    <D> is the minimum number of seconds after estimated S arrival to
#        start the peak search.
# default: searchWindow 0.0 2.0 0.0 30.0 # Search window runs from 2 seconds
#        before to 30 seconds after the estimated S arrival, independent of
#        the P - S interval.

searchWindow 0.0 2.0 0.0 30.0

# snrThresh T
# Minimum accepted signal-to-noise threshold....traces are rejected unless
# this threshold is passed.
# The noise level is determined by averaging values before the estimated
# P phase arrival time.  Signal level from the average value during the
# search window (described above).
# Default: 3.0

snrThresh 3.0


#saveTrace: whether and how to save synthetic ground-motion traces
#            If saveTrace command is absent, no traces will be saved.
#   Choices: SAC <baseDir> <dirFormat> <filename-format> - 
#                  save gm traces in SAC-format files. 
#                baseDir specifies a fixed directory name.
#                dirFormat gives a formattted subdirectory name; the
#                  format string is that used for the "date" command applied
#                  to the event origin time, with the addition of '%i' 
#                  to indicate the event ID. Be careful with this; some
#                  formats produce output not compatible with file names.
#                filename-format gives the formatted file name using
#                  % to introduce one of the format specifiers "sScCnN%"
#                  's' is the station name in lower case; 'S' is upper case
#                  Likewise for component and network names; '%' stands for
#                  itself. All other characters are taken literally.
#
#   default: None (no traces are saved.)


# XMLDir dirname
# Optional command to cause ground-motion results to be written to
#   a ShakeMap XML file. This directory must already exist.
XMLDir Shake/input

# TempDir dirname
# Temporary directory for writing XML file. If XMLDir is given, TempDir must
#   also be given. On Solaris (Unix), these two directories should be on 
#   the same filesystem. This will ensure that the XML file will be complete
#   as soon as it is moved into XMLDir. This directory must already exist.
TempDir Shake/temp

# MappingFile filename
# To provide descriptive names in the shakemap XML file, a mapping file
#   may optionally be given. This file contain mappings of SCN to instrument 
#   type, SN to long station names, and N to agency names.
MappingFile   PNSN.map              
                                    

# Debug N
# There are several different debug features, listed below. 
# You can give one Debug command for each feature you want to turn on, 
# or you can add together the values of desired features and give 
# one Debug command.
# value   feature
#     1   trace and search times and P and S arrival estimates
#     2   SCN selection tests
#    16   ws_client debugging
#    32   poles. zeros and gain values
#    64   trial frequency response functions to STDOUT
#   128   full frequency response function to STDOUT


# SCN Parameters: normally this command will be given as "@scn_param_file"
# to cause the separate file "scn_param_file" to be processed.

# Optional parameter to use with eqassemble which declares the version
# of an event such as Prelim, Rapid and Final. Possible values for the
# parameter LookAtVersion are: All, Prelim, Rapid and Final.
# Default is All.
# LookAtVersion All


## OPTIONAL PARAMETER
## sendActivate: When present this parameter sends an activated_scripts
# message with the eventID as the first argument and written XML file as the
# second argument. This is convenient when you want to run a script (such as 
# the ShakeMap suite) after the XML is created. This is optional.
#
#sendActivate

## OPTIONAL PARAMETER
# this is the logo to use for the sendActivate message, 
# if it is not set, then the logo is the name of the gmew module
#
# sendActivateLogo MOD_ACTIVATED_SCRIPTS

## OPTIONAL PARAMETER
## PreTriggerSeconds: Used to compute start time, P and S for Triggered events; 
# defaults to 10
#
# PreTriggerSeconds   30

