#              This is EWPublish's Parameter File
#
MyModuleId  	MOD_EWPUBLISH   # Module id for this instance of ewpublish 
RingName    	PICK_RING      	# Ring to use for input and output ring 
				# for the <selected solution>, 
				# i. e. the first solution issued by binder from L1 locator
TestRing    	TEST_RING	# Ring to export every hyp2000arc msg (optional) 
LogFile     	1              	# 0 to completely turn off disk log file
maxsite     	1000            # Maximum stations in station list
HeartBeatInt	60.0		# Heartbeat interval, in seconds



# Parameters for Magnitude estimation from Pd 
#    MagPd a0 a1 a2 a3, where:
#
# 		a0	Pd|200  = Pd * 10^( -a0 * log10(200/dhypo) )    => a0 = 1.5
# 		a1,a2	log10(Pd|200)  = a1 *  Mag - a2                 => a1,a2 = 1.0,8.4  
#
#		a1 can not be .0
# 
MagPd		1.7 1.0 8.3


# Parameters for Magnitude estimation from Tc 
#    MagTauC b1 b2 b3, where:
#
# 		b1,b2	log10 TauC = b1 * Mag - b2  	     => b1,b2 = 0.25,1.2 
#
#		b1 can not be .0
# 
MagTauC		0.3   1.6

# Final magnitude comptutation from MagPd and MagTauC
# 		c1,c2   Mag = c1 * MagPd + c2 * MagTauC  => c1,c2 = 0.5,0.5  
#
# 		c1 + c2 must be 1
#
Mag 		1. 0.

# Maximum epicentral distance accepted to compute proxies, in km 
#
DepiMax		300 

# Minimum value from Pd accepted to publish with the event, in cm  
#
PdMin		1e-5 



# Type of the magnitude: L, W, B, S, D will be processed by orareport
# 
MagType 	W 


# Parameters for depi (and residual times) computation
site_file	"UserStations.hinv"
@C:\DAS\params\model.d 



# The message logos to grab from transport ring
#              Installation       Module          Message Types
GetPicksFrom   INST_WILDCARD   MOD_WILDCARD
GetAssocFrom   INST_WILDCARD   MOD_WILDCARD