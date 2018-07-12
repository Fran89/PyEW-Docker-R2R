# ewhtmlreport configuration file


# Earthworm stuff
 MyModuleId     MOD_EWHTMLREPORT
 InRing         HYPO_RING
 HeartbeatInt   30
 LogFile        1
 Debug          1

 
 MaxMessageSize	16384
 site_file		memphis2.hinv
 ArcFolder		arc
 MagFolder		mag
 HTMLFile		temp/report		# where the periodic report gets written
 HTMLBaseFile		ewhtmlreport.html	# the location of the base .html file
 WriteIndex		temp/index.html		# OPTIONAL command to also write the HTMLFile to a statically named file.
 ReportInt		1.0		# Interval between reports, in days (defaults to 1 day, can be fraction)
 ReportTime		-1		# UTC time of the report 0 - 23. -1 means send at start time
 ReportPeriod	200		# Number of days considered for each report - Beware that this amy lead to VERY large files (60)
 MaxDist		10		# Maximum distance considered for the average event count per station (10)

 CSVBasename            temp/excel_data  # the basename (optional) to produce a comma separated values (csv) format file of the events

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
 
# SubjectPrefix - Optional to set the prefix of the subject to customize the email. Defaults to EWalert if this is not set.

# List of email recipients
 EmailRecipient test@mail.com

MinQuality C   # - optional new setting to not include any events in catalog BELOW this quality flag, 
	       # A,B,C possible, listing D is like not including the option
