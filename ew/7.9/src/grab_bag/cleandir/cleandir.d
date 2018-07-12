##############################
# cleandir.d
#
# This specifies the parameters for cleandir.
# Age - number of days old a file has to be before it is deleted
#	Default is 100 days
# CheckInterval - time interval between checking the directories (in seconds)
#	Default is 14400 seconds (4 hours)
# Directory - directory to check.  Up to 16 directories can be specified,
# 	each on a separate line.  This will not navigate into subdirectories.
##############################

Age		100
CheckInterval	14400
Directory	C:\tmp
Directory	C:\Temp
Directory	$EW_LOG

