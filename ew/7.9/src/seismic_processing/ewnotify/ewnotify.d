#
#         enotify Configuration File 
#
MyModuleId   MOD_ENOTIFY  # this module's id

InRing     TEST_RING	# Transport ring to read messages from
OutRing    HYPO_RING	# Transport ring to write messages to
MaxMsgSize 50000		# Maximum message size

# Debug                 # If present, include debugging info in log

ScriptModule MOD_ACT_SCR # Instance of ACTIVATED_SCRIPTS to trigger

SendMailTo	acct1@gmail.com
SendMailTo	acct2@gmail.com
MailServer	smtp.gmail.com
SendMailFrom	acct3@gmail.com
MailProg        /usr/bin/mail

AlertSubject	EWNotify: Alert message
TestSubject		EWNotify: Test signaled
