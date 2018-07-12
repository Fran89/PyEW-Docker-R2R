#    #  (comment)
#    Any line that starts with '#' is treated as a comment.  
#    There does not need to be a <space> after the <pound>, 
#    as was the case in versions <= 1.25.
#

# server
#    This is the IP address and port of the wave_server that wave_viewer 
#		 should connect to.
#    The format is "XXX.XXX.XXX.XXX PORT#"
#

# Wave_Server
server 127.0.0.1 16030



# start
#    Sets the start time of the display (seconds since 1970).
#

# scale
#    Sets the scale is the maximum +/- value that can be displayed
#    for each band.  If a value in the trace is greater than the 
#    scale, it will be truncated to the scale value.  The "scale"
#    config command provides a method for making the scale values
#    for all channels the same.  
#
#scale 500


# band
#    Sets the starting height of each band in pixels.  Wave_viewer will
#    attempt to fit as many bands on the screen as possible, given the
#    band height.
#
band 50


# TimeTillUnblock
#    Time in seconds that wave_viewer will wait before unblocking 
#		 blocked traces.  It can be useful to set this value to a low 
#		 number (5), if you are trying to view trace at the bleeding 
#		 edge of the tank.  
#		 WARNING!! the lower the number is set, the larger the load
#    placed on wave_viewer and it's corresponding WAVE SERVER will be.  
#		 So please try to use a number closer to 25 (the default) if you 
#		 are not needing to look at bleeding edge data.
#
# TimeTillUnblock 5


# auto_scroll
#    When in auto_scroll mode, this is the automated window scrolling 
#		 interval (in seconds).  
#		 NOTE:  due to other constraints, the minimum scrolling value 
#		 is 0.5 seconds.  Setting this value below 0.5 will not cause 
#		 wave_viewer to scroll more often.  Setting this value above
#    0.5 will cause wave_viewer to scroll less frequently.
#
# auto_scroll 1.0


# gulp
#    The maximum amount of trace (in seconds) to request from 
#    wave_server in a single request.  Wave_viewer will never 
#    ask for more than "gulp" seconds of data in a single 
#    request.  If wave_viewer wants 60 seconds of data, and 
#    "gulp" is set to 10, then it will make 6 "10 second" 
#    requests to get the 60 seconds of data.
#
gulp 10


# cache
#    This is the local cache size in # of samples.  So if you 
#    have 200hz data, and you set the cache size to 20000, then 
#    wave_viewer will cache 100 seconds of data.  If you had 
#    1 hz data then wave_viewer would cache 20000 seconds of data.
#    WARNING!!  The total amount of memory that wave_viewer will 
#    attempt to grab is equal to 3MB + (# of channels in the given 
#    wave_server) * (cache size) * (sizeof(int)).  So if you
#    connect to a wave_server that serves 100 channels, and you set 
#    cache to 20000, then memory usage should be close to 
#    3000kb + (100 * 20k * 4b) = 11000kb = 11MB.  (4 is sizeof(int))
#
#    20000 is 200 seconds worth of 100hz data.  (80kb/channel)
cache 20000  


# max_trace
#    Maximum number of traces that wave_viewer will handle.  If a 
#    wave_server serves more than <max_trace> channels, then 
#    wave_viewer will pay attention to only the first <max_trace>
#    channels, ignoring the rest.
max_trace 128


# group
#    Denotes the beginning of a block of lines that describe a group 
#    of channels.  Groups can then be selected from the wave_viewer 
#    menu bar, in order to view only a subset of channels, in the
#    order specified in the config file.  When specifying groups in 
#    the config file, use the following format:
#    group <GROUP_NAME>
#     <STA1> <CHAN1> <NET1> [<LOC1>]
#     <STA2> <CHAN2> <NET2> [<LOC2>]
#     <STA3> <CHAN3> <NET3> [<LOC3>]
#    <BLANK LINE WITH NO SPACES>
#

group Z_Sites
        CH00 SHZ SR P4
        CH03 SHZ SR P4

group 012_Sites
        CH00 SHZ SR P4
        CH01 SHE SR P4
        CH02 SHN SR P4

group All
        CH00 SHZ SR P4
        CH01 SHE SR P4
        CH02 SHN SR P4
        CH03 SHZ SR P4
        MARK GPS SR P4
