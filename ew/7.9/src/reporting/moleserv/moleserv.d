################################################################################
# Configuration file for Moleserv                                              #
# When in standalone mode, earthworm-related options are ignored               #
################################################################################

MyModuleId		MOD_MOLESERV
LogFile			1
Debug			0
InRing			PICK_RING
HeartbeatInt	30

# For webserver options, see mongoose help at
# https://github.com/valenok/mongoose/blob/master/UserManual.md
WebServerOpt	listening_ports	0.0.0.0:8089
WebServerOpt	error_log_file  ./mongoose.log
WebServerOpt	document_root	./html

MoleDB			${MOLEDBNAME}
MoleUser		${MOLEDBUSER}
MolePass		${MOLEDBPASS}
ODBCName		MySQLMole

XMLUri			mole			# Uri used to get xml data. Optional. Default is mole
evtParamPubID	MoleServ		# public ID for eventParameters. Optional. Default is MoleServ
XMLbuflen		100000			# Buffer length for XML queries


