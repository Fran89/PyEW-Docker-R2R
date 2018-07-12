#
# This is the heli_ewII parameter file. This program gets data gulps
# from the waveserver(s), and creates helicorder displays.
# Derived from heli_ew and heli_standalone, which were derived from
# Jim Luetgert's original efforts.

# This program runs either standalone or as an Earthworm module.
# If any of the four Earthworm paramaters below are stated, it will asssume
# that it's supposed to be a module. It will beat it's heart into the ring,
# and use the Earthworm logging scheme. Other wise, it runs standalone.
LogSwitch 1
MyModuleId MOD_HELI_EWII
RingName ASSOC_RING
HeartBeatInt	10

wsTimeout 30 # time limit (secs) for any one interaction with a wave server.

# List of wave servers (ip port comment) to contact to retrieve trace data.
 WaveServer 136.177.31.188 16022      "harry"

# Directory in which to store the temporary .gif, .link and .html files.
GifDir   C:\alex\heli_alex\Gifs\

# Plot Parameters - sorry it's so complex, but that's the price of versatility
        # The following is designed such that each SCN creates it's own
        # helicorder display; one per day of data.
# S                  Site
# C                  Component
# N                  Network
# L		     Location Code
# 04 HoursPerPlot    Total number of hours per gif image
# 05 Plot Previous   On startup, retrieve and plot n previous hours from tank.
# 06 Local Time Diff UTC - Local.  e.g. -7 -> PDT; -8 -> PST
# 07 Local Time Zone Three character time zone name.
# 08 Show UTC        UTC will be shown on one of the time axes.
# 09 Use Local       The day page will be local day rather than UTC day.
# 10 XSize           Overall size of plot in inches
# 11 YSize           Setting these > 100 will imply pixels
# 12 Minutes/Line    Number of minutes per line of trace
# 13 Gain Factor     Instrument gain in microvolt/bit.
# 14 Scale Factor    Scale factor to dress up image.
# 15 Mean Removal    Mean of 1st minute of each line will be removed.
# Comment            A comment for the top of the display.
#                                      
#     S    C   N  L 04 05  06  07   08 09 10  11  12 13   14   15  Comment

 Plot AHID BHE US -- 12 1   -8  PST  1  0  20  20  15 0.1  1.5  1   "Alston BPA"
 Plot ADK  SHZ AT -- 12 1   -8  PST  1  0  20  20  15 0.1  1.5  1   "Mt. Rainier Summit"


    # *** Optional Commands ***

 Days2Save     7    # Number of days to display on web page; default=7
# Actually, "Days2Save" is really the number of GIFs of each SCN to save.
# For example, if HoursPerPlot is 12, you will save 7 gifs for 3.5 days.

 UpdateInt    10    # Number of minutes between updates; default=2
 RetryCount    2    # Number of attempts to get a trace from server; default=2
# Logo    smusgs.gif # Name of logo in GifDir to be plotted on each image

# We accept a command "Clip" which sets trace clipping at this many
# vertical divisions
Clip 5

# We accept a command "SaveDrifts" which logs drifts to GIF directory.
# SaveDrifts

# We accept a command "Make_HTML" to construct and ship index HTML file.
Make_HTML     

# We accept a command "IndexFile" to name the HTML file.
# Default is "index.html"
IndexFile welcome.html

# We accept a command "BuildOnRestart" to totally rebuild images on restart.
BuildOnRestart     

# We accept a command "Debug" which turns on a bunch of log messages
Debug
WSDebug

# MaxSampRate is option, will default to 500 is not used
# MaxSampRate 500
