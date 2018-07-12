# Configuration file for the UW version of the localmag program
# Some of these parameters can be overridden on the command-line;
# some of them only make sense on the command-line.
#
# As of 20 December, 2000, there is NO SUPPORT for Earthworm Database (EWDB)
#
# Required Commands: 
# The following five commands are always required by localmag.
# Other commands below may be required when certain options are given.
#
# maxSta number
# The number of stations to be used by localmag must be specified with
# the maxSta command. This is the number of unique SNs to be used.
# A total of 6 * maxSta channel structures will be allocated.
# The maxSta command must come before any "Pri" commands.

maxSta 10

# maxDist km
# The maximum epicenter - station distance in kilometers. 
# Stations lying outside this radius will not be used by localmag.
#
maxDist 600


# maxTrace nsamples
# The maximum number of trace data samples to process from one SCN.
# This is the product of the trace length in seconds and the highest 
# expected sample rate. For example, 10 minutes of data at 100 samples per
# second gives 60000 samples.

maxTrace 60000


# logA0 filename
# The file containing the table of -logA0 values versus distance.
# This file mist start with two commands:
#   Dist <type>: which distance is to used to access this table.
#        choices are: epi - station - epicenter distance
#                     hypo - station - hypocenter distance (use event depth
#                          but not station depth.)
#   nDist number: the number of table entries that follow.
#  Following these two commands are `nDist' lines of distance and -logA0 
#  values. The distance numbers must be integer values, in kilometers.
#  The -logA0 values are positive decimal values.
#  Entries MUST be in order of increasing distance.
#  When this table is used to find the -logA0 value, the entry with distance
#  nearest to the desired distance will be used. If the desired distance
#  exceeds the largest distance in this table, then the -logA0 value for
#  the last table entry will be returned.

logA0 Richter.tab


# staLoc: station location source
#    Choices: File <loc-file> - get station locations from a hyp2000-format
#                               file named loc-file
#             EWDB - query the EW database for station locations; access info
#                    given below.
#             SAC - get station locations from the header of SAC files.
#                   Can be either station lat and lon, or station-epicenter
#                   distance in the SAC header.
#             UW - get station location from the standard wash.sta file

staLoc UW


# Regional Velocity model: must be specified as "@vel_model_file". Typically
#    this will be the same file as used for binder_ew.

@uw_velocity.d

# Speed of Sg wave; provides a good estimate of the Wood-Anderson peak arrival

SgSpeed 3.39

# Optional Commands
#
# eventSource: Tells localmag how and where to learn about a new event
#    choices are: ARCH - hyp2000 archive message
#                 SAC - SAC file header; 
#                 EWDB <eventID> - query EW database for eventID; usually
#                            given as command-line option.
#                 UW <pickfile> - read UW-format pickfile; usually given
#                            as command-line option.
#    default: ARCH
# eventSource ARCH


# traceSource: Where localmag will obtain trace data
#    choices are: waveServer <server-list> - raw traces from wave_servers; 
#                              server-list is either a space separated list
#                              of ip-address:port or 
#                           "File <server-file>"
#                 SACFile - raw traces from SAC files
#                 SACWAFile - synthetic Wood-Anderson traces from SAC files
#                 EWDB - raw traces from EW database; access info given below
#                 UWData - raw traces from UW-format datafile corresponding
#                            to the UW-format pickfile.
#                 UWWAData - synthetic Wood-Anderson traces from UW-format
#                            datafiles
#    default: waveServer with list of servers read from $EW_PARAMS/servers

# traceSource waveServer File servers


# Add STA COMP NET
# Del STA COMP NET
# SCN selection is done by a selection list and a rejection list.
# The station selection command `Add' specifies the names of 
# STA (station), COMP (component), and NET (network). 
# The wildcard `*' may be used for any or all of STA, COMP, NET.
# COMP may be given as the first two letters of the component name,
# to select all direction desgnators are accepted for that component.
# If no Add commands are given, all SCNs will be selected.
# The Del command is used to put SCNs on the rejection list:
# for example: "Add * EL UW" followed by "Del LAB EL UW" to include
# all ELE and ELN components from UW except LAB.ELE.UW and LAB.ELN.UW.
# Localmag will NEVER use vertical components.
# List as many Add and Del commands as desired, one to a line

Add * BH *
Add * HH *
Add * SL *
Add * EL *
Del LAB * UW


# respSource: Tells localmag where to get response information; 
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
#             UW - get response information from the standard calib.sta file
#                  and files in calib.poles_zeros.
# 
#    default: none; you must specify one of the above choices.

respSource UW


# readAmpDirect: flag to tell localmag to read Wood-Anderson amplitudes
#   directly from a `trace' source instead of from Wood-Anderson traces. 
#   If this flag is used, then traceSource must be one that includes 
#   amplitude data, such as SAC files, EW database, or UW-format pickfiles.
#
# default: flag is not present, so localmag reads amplitudes from
#   Wood-Anderson traces that it either synthesizes or reads from traceSource.


# traceTimes start end
# The start and end times for traces is set here. 
#   <start> is the number of seconds before estimated P arrival to start
#           the trace.
#   <end> is the number of seconds after estimated S arrival to end the trace.
#   default: traceTimes 5.0 60.0 # starts trace 5 seconds before estimated P
#           arrival and ends trace 60 seconds after estimated S arrival.

traceTimes 5.0 60.0


# extraDelay seconds
# number of seconds to wait after the total trace length has arrived at
# most distant station before processing event; used only when localmag is
# running as an Earthworm module.
# default: extraDelay 0.0  # no extra delay added to estimated propagation time

extraDelay 0.0

# searchStartPhase phase
# The phase to use for computing the start of the peak serach window
#    Choices: P - the first-arriving P phase from the layered velocity model
#             S - the first-arriving S phase from the layered velocity model
#    default: searchStartPhase S

searchStartPhase S


# searchTimes A B
# The time window for searching for peak amplitudes is set here.
#    <A> number of seconds before the searchStartPhase to start the
#        search window.
#    <B> is the  number of seconds of seconds after the constant-speed Sg
#        arrival to end the search window
# default: searchWindow 1.0 45.0 # Search window runs from 1 second
#        before searchStartPhase arrival to 45 seconds after Sg arrival

searchTimes 1.0 45.0


# slideLength n
# Set the width of of the sliding search window to n seconds.
# default: slideLength 0.8 # sliding search window is 0.8 seconds wide

slideLength 0.8



# z2pThresh is the threshold by which the zero-to-peak maximum in the
# search window must exceed twice the zero-to-peak maximum from the
# pre-event portion of the trace. This test tries to ensure that the
# zero-to-peak and peak-to-peak values are seismic and not just noise.
# default: z2pThresh 3.0

z2pThresh 3.0

# meanCompMags: If present, this command causes localmag to compute a 
#    stations local magnitude by taking the mean of the local magnitudes
#    for each of its horizontal components, Otherwise, localmag will take
#    the average of the component amplitudes and use that to find the
#    local magnitude for the station.


# WoodAndersonCoefs <period> <damp> <gain>: optional command to specify the
#   coefficients used for the Wood-Anderson instrument response. The standard
#   parameters for the transfer function are: 
#   period: 0.8 seconds; damping 0.8 critical; gain: 2800
#   However, testing by Uhrhammer & Collins (BSSA 1990, V80 p702-716) and
#   others indicate better values are:
#   period 0.8 seconds; damping 0.7 critical; gain 2080 (twenty-eighty)
#
# default: WoodAndersonCoefs 0.8 0.8 2800


# saveTrace: whether and how to save synthetic Wood-Anderson traces
#            If saveTrace command is absent, no traces will be saved.
#   Choices: SAC <baseDir> <dirFormat> <filename-format> - 
#                  save W-A traces in SAC-format files. 
#                baseDir specifies a fixed directory name.
#                dirFormat gives a formattted subdirectory name; the
#                  format string is that used for the "date" command applied
#                  to the event origin time, with the addition of `%i' 
#                  to indicate the event ID. Be careful with this; some
#                  formats produce output not compatible with file names.
#                filename-format gives the formatted file name using
#                  % to introduce one of the format specifiers "sScCnN%"
#                  `s' is the station name in lower case; `S' is upper case
#                  Likewise for component and network names; `%' stands for
#                  itself. All other characters are taken literally.
#
#            UW - save W-A traces in UW-format datafiles with ".wa." added
#                 to the named before the file tag.
#
#   default: None (no Wood-Anderson traces are saved.)


# outputFormat: how localmag reports its results; results are always logged
#    Choices: LM - TYPE_LOCALMAG message to earthworm transport
#             EWDB - send results to Earthworm database.
#             UW - write results in UW-format pickfile
#  default: LM if using earthworm transport; no report otherwise


#
# EWDBaccess user password service  
#            How to access the Earthworm Database, if needed. 
#            Give the user name, password, and srvice needed for connection.
# default: none


# SACsource <SAC-directory> <filename-format>: where to find SAC files for
#                            reading.
#                            <SAC-directory> is a fixed directory name
#                            <filename-format> is the format for SAC
#                            file names.
#                            This command or its equivalent on the command
#                            line is required if traceSource or respSource
#                            is SAC files.

SACsource /data0/earthworm/working/src/localmag/Test/98042703361 %S.%C


# wsTimeout: wave_server timeout in milliseconds
#    default: 5000 milliseconds

# wsTimeout 10000


# Earthworm Transport commands. Use these commands to make localmag
#   run as an earthworm module to run continuously.
#   Otherwise localmag runs as a standalone program and handles a
#   single event per invocation.
#
# RingInName <ring>: specify the name of the earthworm transport ring from
#  which to read TYPE_HYP2000ARC messages for event notification.

# RingInName HYPO_RING

# RingOutName <ring>: specify the name of the earthworm transport ring to
#  which Magnitude, Heartbeat and Error messages are sent. This may be
#  the same as RingInName if desired.

# RingOutName HYPO_RING

# MyModId <module-id>: specify the module ID for localmag
# MyModId MOD_LOCALMAG

# HeartBeatInterval int: How often localmag should beat its heart.
#  Currently, localmag will not issue heartbeats while it is busy doing
#  calculations for an event. Thus a heartbeat could be late by as much as the
#  time it takes to do a full localmag event calculation. Be sure that
#  statmgr will wait that long for the heartbeat.

# HeartBeatInterval 30

# getEventsFrom  INST_ID MOD_ID
# Specify the installation ID and module ID names from which to get
# hypoinverse archive messages to read event data. These names may be
# selected from earthworm.d and earthworm_global.d Only required when
# localmag is running as an Earthworm module. No default values.
# Use as many of these commands as necessary.

# getEventsFrom INST_WILDCARD MOD_WILDCARD


LogFile 1         # 1 -> Keep log, 0 -> no log file
                  # 2 -> log to module log but not to stderr/stdout
                  # default: LogFile 1


# Debug N
# There are several different debug features, listed below. 
# You can give one Debug command for each feature you want to turn on, 
# or you can add together the values of desired features and give 
# one Debug command.
# value   feature
#     1   trace and search times and P and S arrival estimates
#     2   SCN selection tests
#     4   distance and LogA0 values
#     8   SAC file selection (only if traceSource is SAC)
#    16   ws_client debugging
#    32   poles. zeros and gain values
#    64   trial frequency response functions to STDOUT
#   128   full frequency response function to STDOUT
#   256   input and output trace date including in padded area
# Debug 1

# SCN Parameters: normally this command will be given as "@scn_param_file"
# to cause the separate file "scn_param_file" to be processed.
