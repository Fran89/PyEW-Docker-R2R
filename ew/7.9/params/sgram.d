#
# This is the spectrogram parameter file. This module gets data gulps
# from the waveserver(s), and computes spectrograms to be displayed in gif.

#  Basic Earthworm setup:
#  The only thing that the RING connection is used for is for statmgr control.
#  If the argument "StandAlone" is set then no earthworm need be running but
#  the following four parameters must still be set.
#
LogSwitch     1           # 0 to completely turn off disk log file
MyModuleId    MOD_SGRAM   # module id for this instance of report 
RingName      HYPO_RING   # ring to get input from
HeartBeatInt  600          # seconds between heartbeats

# To not use earthworm control set StandAlone.
# Comment out to use statmgr control.
StandAlone

wsTimeout 20 # time limit (secs) for any one interaction with a wave server.

# List of wave servers (ip port comment) to contact to retrieve trace data.
WaveServer aaa.bbb.ccc.ddd 16021      "UW"
WaveServer aaa.bbb.ccc.ddd 16022      "UW"
WaveServer aaa.bbb.ccc.ddd 16023      "UW"
WaveServer aaa.bbb.ccc.ddd 16024      "UW"
WaveServer aaa.bbb.ccc.ddd 16025      "baker UW"

#  WARNING - GifDir and LocalTarget directories MUST be on the same filesystem.
# List of target directories to place output files.
 LocalTarget /spikehome/HTML/SEIS/PNSN/WEBICORDER/SGRAM/

# Working directory in which to store the temporary .gif, .link and .html files.
# MUST be on the same filesystem as LocalTarget and end in '/'
 GifDir   /spikehome/HTML/SEIS/PNSN/WEBICORDER/TMPGIF/

# Filename prefix on target computer.  This is useful for identifying
# files for automated deletion via crontab.
 Prefix uw

# Plot/SCNL  Parameters - sorry it's so complex, but that's for versatility
        # The following is designed such that each SCN/SCNL creates it's own
        # spectrogram display; one per day of data.
# S  Site
# C  Component
# N  Network
# L  Location code   (if command is "SCNL", if it is "Plot" then leave this out)
# 04 Hours/Plot      Number of hours in each GIF image.
# 05 Plot Previous   On startup, retrieve and plot n previous hours from tank.
# 06 Local Time Diff UTC - Local.  e.g. -7 -> PDT; -8 -> PST
# 07 Local Time Zone Three character time zone name.
# 08 Show UTC        UTC will be shown on one of the time axes.
# 09 Use Local       The day page will be local day rather than UTC day.
# 10 XSize           Size of plot in inches. Setting this > 100 will imply pixels
# 11 Pixels/Line     Vertical pixels per line of trace displayed
# 12 Minutes/Line    Number of minutes per line of trace displayed
# 13 Seconds/Gulp    Number of seconds per fft
# 14 Freq Max        Maximum frequency
# 15 Freq Mute       Hz to mute at low end
# 16 nbw             Display scaling. 1:linear, 2:log 3:log(1st diff) [neg for greyscale]
# 17 amax            Clipping amplitude applied to spectrum [0 for none]
# 18 scale           Scale factor for wiggle-line display.
# 19 Comment         A comment for the top of the display.
#                                      
#     S    C   N  04 05 06  07  08 09 10   11  12 13 14 15    16  17        18  Comment
 SCNL CPW  EHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   5000     0.1 "Capital Pk."
 SCNL SMW  EHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   10000     0.1 "South Mt"
 SCNL FMW  EHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   10000     0.1 "Mt Fremont, Rainier"
 SCNL JCW  EHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   20000     0.1 "Jim Creek, WA"
 SCNL GNW  BHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.5   1   10000     0.1 "Green Mt. BB"
 SCNL HDW  EHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   10000     0.1 "Hoods Port, WA"
 SCNL BLN  EHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.5   1   40000     0.1 "Blyn BB"
 SCNL B001 EHZ PB -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   20000     0.1 "Sequim borehole PBO"
 SCNL SQM  BHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   40000     0.1 "Sequim, WA BB"
 SCNL B007 EHZ PB -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   20000     0.1 "Shore Borehole PBO"
 SCNL STW  EHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   30000     0.1 "Striped Pk, WA"
 SCNL LRIV BHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.5   1   20000     0.1 "Little River, Olympics"
 SCNL MCW  EHZ UW -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   10000     0.1 "Mt Constitution, WA"
 SCNL SNB  BHZ CN -- 12 6  -7  PDT 1  0  600  1   1  60 10 0.3   1   40000     0.1 "Saturna Isl. BC"


    # *** Optional Commands ***

 Days2Save     7      # Number of days to display on web page; default=7
 UpdateInt     5      # Number of minutes between updates; default=2
 RetryCount    2      # Number of attempts to get a trace from server; default=10
 Logo    pnsn_logo2.gif   # Name of logo in GifDir to be plotted on each image

# We accept a command "SaveDrifts" which logs maximum values to GIF directory.
 SaveDrifts

# We can make our plots go top to bottom rather than up
 PlotDown

# We accept a command "Make_HTML" to construct and ship index.html file.
Make_HTML     

# We accept a command "BuildOnRestart" to totally rebuild images on restart.
# BuildOnRestart     

# To back file seismograms earlier than today we accept:
# DaysAgo 4

# We accept a "Days2Save" paramter with argument between 1 and 14 for how
# long the list of previous spectrograms can get.  Default is 7.
# Days2Save 12

# We accept a command "Debug" which turns on a bunch of log messages
# Debug
# WSDebug
