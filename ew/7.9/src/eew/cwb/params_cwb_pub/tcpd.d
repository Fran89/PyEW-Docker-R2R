
# This is template's parameter file

#  Basic Earthworm setup:
#
MyModuleId         MOD_TCPD  # module id for this instance of template 
RingName           PICK_RING   # shared memory ring for input picks
RingName_out       EEW_RING    # shared memory ring for output EEW information

LogFile            0           # 0 to turn off disk log file; 1 to turn it on
                               # to log to module log but not stderr/stdout
HeartBeatInterval  15          # seconds between heartbeats


MagMin 0.0 			    # lower bond magnitude for report
MagMax 10.0		        # Upper bond magnitude for report

Ignore_weight_P		2  # include 3
Ignore_weight_S		2

Mark	231 	# 3 characters for identify system (depend by yourself)

MagReject			CHGB HHZ BS 01		# which channle will be ignored for magnitude estimation


#------------ For Picks association
Trig_tm_win       40.0		# The P wave arrival time between each triggered station
Trig_dis_win      180.0		# Distances between each triggered station
Active_parr_win   45.0		# Survival time of each channel (sec) , elapse time between the P wave arrival time and the current time



Term_num     50          		          #  The last report should be less than this number.                     
Show_Report	  1							  #  0: Disable, 1:Enable, log files for each EEW reports

#----------------- P-wave velocity model :  
#	 Velocity(Depth) =  SwP_V + SwP_VG * Depth   (if Depth less than Boundary_P)
# 	 Velocity(Depth) =  DpP_V + DpP_VG * Depth   (if Depth larger than Boundary_P)

  Boundary_P     40.0        		          # boundary of shallow and deep layers                                    
  SwP_V        5.10298          		      # initial velocity in shallow layer                                    
  SwP_VG       0.06659       		          # gradient velocity in shallow layer                                   
  DpP_V        7.80479         		          # initial velocity in deep layer                                       
  DpP_VG       0.00457      		          # gradient velocity in deep layer                                      





# List the message logos to grab from transport ring
#              Installation       Module		Message
GetEventsFrom  INST_WILDCARD    MOD_WILDCARD	TYPE_EEW







