#
# coaxtoring configuration file
#
#   The setup is different for Solaris and NT. The specified address
#   will be used to receive class c subnet broadcasts.
#
# Solaris:    InAddress must be of the form xxx.xxx.xxx.255
# Windows NT: InAddress must be the complete local address, eg
#             192.168.4.107
#
MyModuleId   MOD_COAXTORING_A   # Module id for this process
RingName     WAVE_RING          # Write to this shared memory region
LogFile      1                  # 1=write log file on disk; 0=don't
                                # 2=write to module log but not stderr/stdout
nMsgBuf      16                 # There should be at least one buffer per logo
MsgMaxBytes  4096               # Size of biggest message to be received
InAddress    192.168.4.255      # IP address of ethernet line to read

PortNumber   10001              # Under Solaris, if the port number is at
                                #  or below 1024, the program must run as root.

ScrnMsg      1                  # 1=print messages on screen; 0=don't
HeartbeatInt 10                 # Heartbeat interval in seconds
BufferReportInt 300             # Print high buffer count every n seconds

RcvBufSize   260000             # Size of socket receive buffer, in bytes.
                                # Set RcvBufSize to a large number to avoid
                                #   dropping packets.
                                # Max value is system dependent
                                #  (typically 260000)
                                # If you set =0, it will default to the 
				# system default. (typically 8192)
