# ewhtmlemail configuration file


# Earthworm stuff
 MyModuleId     MOD_EWHTMLEMAIL          # Module ID
 InRing         HYPO_RING
 HeartbeatInt   30                   # Heartbeat interval in seconds
 LogFile        1                    # If 0. don't write logfile; if 1. do
                                     # if 2. write module log but not stderr/stdout
 Debug          0
 DebugEmail     0		# echo the email command used if set to 1

# Get messages from
 GetLogo  INST_WILDCARD  MOD_WILDCARD # TYPE_HYP2000ARC

#DataCenter         between quotes (optional and is used in the table header)
# DataCenter  "My data Center"


#CenterPoint 45.0 -121.0  # OPTIONAL lat and lon for EmailRecipientWithMinMagAndDist

# ShortHeader - OPTIONAL flag to compress the header table and leave less whitespace
#

#
# DontShowMd - Optional feature to remove Md from the report.
#
# DontUseMd - Optional feature to not use Md  for email decisions for any EmailRecipientWithMinMag args
#
# UseML - OPTIONAL feature to turn on TYPE_MAGNITUDE (localmag) use
# UseMW - OPTIONAL feature to turn on TYPE_MAGNITUDE and detect MW  magnitude messages
#
# MagWaitSeconds - OPTIONAL wait this long before releasing an 
#                  email that may not have an ML or MW, default 5 seconds
 
# Note that hyp2000arc messages can be very large if the number of stations
# is high.
 MaxMessageSize   16384

# site_file - MANDATORY station file. This corresponds to the station file in
#             hypoinverse-compatible format, as used by hyp2000. It is used to
#             know which waveforms are to be search from the waveservers and
#             also to plot the stations in the static google map.
 site_file    memphis2.hinv
                         


# HTMLFile - MANDATORY base name for the html file of this event. The actual  
#            filename will be <HTMLFile>_<event id>.html
#            This configuration allows creating a folder with html files for the
#            detected seismic events.
 HTMLFile        temp/emailfile

#SPfilter   
SPfilter


# a new option is to produce GMT data
#GMTmap    


# the next four (4) settings govern what GMT format to use
#Two map projections for GMT map: Mercator or Albers. Note: Mercator is the default projection
#Mercator       
#Albers

#Cities 	used when GMTmap is enable
#Cities /home/earthworm/run/params/cities.txt

#StationNames  used when GMTmap is enable
#StationNames

#MapLegend     Used when GMTmap is enable
#MapLegend /home/earthworm/run/params/ewhtmlemail.legend
 


# KML - output (option)
#   SYNTAX for KML OPTION:
#       KML dir preamble_file
# dir is where kml file is written
# preamble_file is where kml preamblefile is read in from, must be the one from the src distro

 
# EmailProgram - Optional email program. It is imperative that the email program
#                is capable of handling html email. ewhtmlemail has been created
#                specifically for sendmail. If this option is commented out, the
#                module assumes that emails are not to be sent and will not 
#                include the mime header in the html file. In this sense, this
#                module becomes an event-to-html converter.
 EmailProgram    /usr/sbin/sendmail

# UseBlat - Optional to say that the EmailProgram is blat type, which takes 
#      syntax "blat.exe file -to recipient"
#   make sure EmailProgram has the absolute path to blat.exe if it is not 
#   in your path, but set the EmailProgram entry regardless

# BlatOptions "-server localhost:1099 -profile gmailsmtp"  - Optional if you need to use stunnel or something for gmail smtp
#           Note this is not even used if you do not have UseBlat configured in the .d file

#UseUTC - Optional to set the time at UTC in the html email messages (localtime zone of PC is default)
 
# SubjectPrefix - Optional to set the prefix of the subject to customize the email. Defaults to EWalert if this is not set.

# ShowDetail - Optional to show a lot more event information in detail, errh, errz, gap, rms etc....

# List of email recipients
 EmailRecipient test@mail.com

 EmailRecipientWithMinMag test2@mail.com 2.0   # only send to this person if mag is >= 2.0

 EmailRecipientWithMinMagAndDist test3@mail.com 2.0 3.0  # only send to this person if mag is >= 2.0 and dist from event <=3km
 

# StaticMapType - optional, can be the maptype for the google static map. 
#		could be: terrain, satellite, or roadmap
#		Default: hybrid (satellite+roads)
 
 
# StyleFile - Optional style file for the generated html file. Some items have
#             specific ids and classes that allows some customization through a
#             css file. Please consult the example ewhtmlemail.css file for the
#             ids and class names.
 StyleFile       ewhtmlemail.css




 ####################### 
 # Waveserver Settings #
 #######################
 MAX_SAMPLES   100000    # Maximum number of samples to extract from any given
                         #    station of the waveserver.
                         #    Default: 60000

 WSTimeout          5    # Waveserver timeout in seconds

# List of waveservers aaa.bbb.ccc.ddd:portno
 WaveServer 192.168.0.1:16022
 WaveServer 192.168.0.2:16022
 
 
 # TimeMargin - Optional parameter to set the time margin for downloading 
 #              waveforms from the waveservers in seconds.
 #              Default: 10.0
 TimeMargin     10.0
 

 # MaxDuration - is the maximum duration of the trace from origin_time - TimeMargin to endtime+ TimeMargin
 # if trace is longer than this in length, then endtime is set to  origin_time - TimeMargin + MaxDuration 
 MaxDuration    60.0   # defaults to 144 seconds, new feature


 #UseGIF				# Not yet functional
 #TraceWidth   1200		# Sets width of traces (Default 600)
 #TraceHeight   200		# Sets height of traces (Default 60)
 # Note that changing width or height is limited to 300 000 points for google charts
 # In addition, the css file should be changed accordingly to avoid resizing
 
                         

# set one letter below: Note quality from hypoinverse is A,B,C,D where A is best, like in grade school!
# MinQuality D		# optional configuration that sets the letter of the minimum hypo quality to send emails
			# this filter is ONLY for email sending. All .html and .kml (if selected) will be created
                         

# UseRegionName 		# OPTION: use flynn-enghdal region names in the table at the top of the email
