
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: carlsubtrig.h 5965 2013-09-23 15:36:11Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2009/08/28 17:48:36  paulf
 *     added TrigIdFilename option for pointing to trig_id.d
 *
 *     Revision 1.6  2004/05/31 17:59:11  lombard
 *     Corrected SCNL lenght macros.
 *
 *     Revision 1.5  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.4  2003/11/17 18:52:22  friberg
 *     Updated carlsubtrig for larger subnets (like Caltech) than just
 *     50 stations. I also added a more intelligent error reporting for
 *     the readsubs.c code when this limit might be reached.
 *
 *     Revision 1.3  2001/03/21 23:13:17  cjbryan
 *     added CVO subnet trigger paramete
 *
 *     Revision 1.2  2000/07/24 20:39:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * carlsubtrig.h: Definitions for the CarlSubTrig Earthworm Module.
 */

/*******							*********/
/*	Redefinition Exclusion						*/
/*******							*********/
#ifndef __CARLSUBTRIG_H__
#define __CARLSUBTRIG_H__

#include <earthworm.h>
#include <time.h>	/* for time_t					*/
#include <transport.h>  /* Shared memory                                */
#include <trace_buf.h>	/* TracePacket					*/

/*******							*********/
/*	Constant Definitions						*/
/*******							*********/

#define CST_VERSION     "v2.0"  /* version of TYPE_CARLSTATRIG_SCNL	*/
#define CSU_VERSION     "v2.0"  /* version of TYPE_TRIGLIST_SCNL	*/
#define NSUBSTA		100	/* Number of stations allowed in subnet	*/
#define INITBUFSIZE	10240	/* Initial size of the trigger message	*/
				/*   buffer.				*/
  /*	Error Codes - Must coincide with carl_trig.desc			*/
#define	ERR_USAGE	1	/* Module started with improper params.	*/
#define	ERR_INIT	2	/* Module initialization failed.	*/
#define	ERR_CONFIG_OPEN	3	/* Error opening configuration file.	*/
#define	ERR_CONFIG_READ	4	/* Error reading configuration file.	*/
#define	ERR_EWH_READ	5	/* Error reading earthworm.h info.	*/
#define	ERR_STA_OPEN	6	/* Error opening station file.		*/
#define	ERR_STA_READ	7	/* Error reading station file.		*/
#define	ERR_SUB_OPEN	8	/* Error opening subnet file.		*/
#define	ERR_SUB_READ	9	/* Error reading subnet file.		*/
#define	ERR_MALLOC	10	/* Memory allocation failed.		*/

#define	ERR_MISSMSG	11	/* Message missed in transport ring.	*/
#define	ERR_NOTRACK	12	/* Message retreived, but tracking	*/
				/*   limit exceeded.			*/
#define	ERR_TOOBIG	13	/* Retreived message too large for	*/
				/*   buffer.				*/
#define	ERR_MISSLAPMSG	14	/* Some messages were overwritten.	*/
#define	ERR_MISSGAPMSG	15	/* There was a gap in message seq #'s.	*/
#define	ERR_UNKNOWN	16	/* Unknown function return code.	*/
#define	ERR_READ_MSG	17	/* Error reading station message.	*/
#define	ERR_PROD_MSG	18	/* Error producing a trigger message.	*/
#define ERR_ORDER	19	/* station trigger received out of order*/
#define ERR_FULLQUEUE	20	/* station trigger queue is full	*/
#define ERR_LATE        21	/* Network clock is too late		*/
#define ERR_THRDSTUCK   22      /* a thread stopped looping             */
#define ERR_SHORT_MSG   23      /* incomplete triglist message sent     */

  /*	Buffer Lengths							*/
#define	MAXCODELEN	8	/* Maximum length of a code.		*/
#define	MAXFILENAMELEN	80	/* Maximum length of a file name.	*/
#define	MAXLINELEN	2048	/* Maximum length of a line read from	*/
				/*   or written to a file.		*/
#define	MAXMESSAGELEN	160	/* Maximum length of a status or error	*/
				/*   message.				*/
#define	MAXTRACELEN	1024	/* Maximum length of a trace data	*/
				/*   buffer.				*/


/*******							*********/
/*	Macro Definitions						*/
/*******							*********/
#define	CT_FAILED( a ) ( 0 != (a) )	/* Generic function failure	*/
					/*   test.			*/

/*******							*********/
/*	Structure Definitions						*/
/*******							*********/

  /*	CarlSubTrig earthworm Parameter Structure (read from *.d files)	*/
typedef struct _CSUPARAM
{
  char	debug;				/* Write out debug messages?	*/
					/*   ( 0 = No, 1 = Yes )	*/
  char	myModName[MAX_MOD_STR];	/* Name of this instance of the	*/
					/*   CarlTrig module -		*/
					/*   REQUIRED.			*/
  char	readInstName[MAX_INST_STR];	/* Name of installation that	*/
					/*   is producing trace data	*/
					/*   messages.			*/
  char	readModName[MAX_MOD_STR];	/* Name of module at the above	*/
					/*   installation that is	*/
					/*   producing the trace data	*/
					/*   messages.			*/
  char	ringIn[MAX_RING_STR];		/* Name of ring from which	*/
					/*   trace data will be read -	*/
					/* REQUIRED.			*/
  char	ringOut[MAX_RING_STR];	/* Name of ring to which	*/
					/*   triggers will be written -	*/
					/* REQUIRED.			*/
  char	staFile[MAXFILENAMELEN];	/* Name of file containing	*/
					/*   station information -	*/
					/* REQUIRED.			*/
  char	subFile[MAXFILENAMELEN];	/* Name of file containing	*/
					/*   subnet information -	*/
					/* REQUIRED.			*/
  int	heartbeatInt;			/* Heartbeat Interval(seconds).	*/
  long	ringInKey;			/* Key to input shared memory	*/
					/*   region.			*/
  long	ringOutKey;			/* Key to output shared memory	*/
					/*   region.			*/

  char  *trigIdFilename;		/* a pointer to the trig_id file name where next_id is stored */
} CSUPARAM;

  /*	Information Retrieved from Earthworm.h				*/
typedef struct _CSUEWH
{
  unsigned char	myInstId;	/* Installation running this module.	*/
  unsigned char	myModId;	/* ID of this module.			*/
  unsigned char	readInstId;	/* Retrieve trace messages from		*/
				/*   specified installation.		*/
  unsigned char	readModId;	/* Retrieve station trigger messages 	*/
				/*   from specified module.		*/
  unsigned char	typeCarlStaTrig;/* Station trigger message type.	*/
  unsigned char	typeTrigList;	/* TrigList message type.		*/
  unsigned char	typeError;	/* Error message type.			*/
  unsigned char typeHeartbeat;	/* Heartbeat message type.		*/
} CSUEWH;

  /* 	A station trigger structure					*/
typedef struct _STATRIG
{
  double	onTime;		/* When the station eta first went 	*/
				/*   above zero				*/
  double	offTime;	/* When station eta <= 0		*/
				/*   0.0 if station eta currently > 0	*/
  double	onEta;		/* Station eta when first above 0	*/
  long		sequence;	/* sequence number (from CarlStaTrig)	*/
} STATRIG;

  /*	Information about a particular station				*/
typedef struct _STATION
{
  char		compCode[TRACE2_CHAN_LEN];	/* Component code.	*/
  char		netCode[TRACE2_NET_LEN];	/* Network code.	*/
  char		staCode[TRACE2_STA_LEN];	/* Station code (name).	*/
  char   	locCode[TRACE2_LOC_LEN];	/* Location code	*/
  double	onTime;			/* Time this station triggered  */
  double	saveOnTime;		/* Time to be saved for message	*/
  double	onEta;			/* Eta value when this station	*/
					/*   first triggered		*/
  double	saveOnEta;		/* Eta value to be saved	*/
  int           coincident_check;       /* 1 if used in check, already */
  int           listMe;                 /* Flag to mark stations that   */
                                        /*   are to be in triglist:     */
                                        /*     1: triggered station not */
                                        /*        in triggered subnet   */ 
                                        /* odd>1: triggered station in  */
                                        /*        triggered subnet(s)   */
                                        /*  even: untriggered station   */
                                        /*        in triggered subnet   */
  int		countDown;		/* station countdown timer	*/
  					/*   (seconds)			*/
  int		timeToLive;		/* How long a station trigger	*/
					/*   lasts after it's eta goes 	*/
					/*   below zero	(seconds)	*/
  int		nextIn;			/* index for the next incoming	*/
					/*   station trigger message	*/
  int		nextOut;		/* index for the next outgoing	*/
					/*   station trigger message	*/
  STATRIG*	triggers;		/* array of pending station	*/
					/*   trigger messages, oldest 	*/
					/*   first, youngest last	*/  
} STATION;

/* Information about a channel (such as time) for triglist */
typedef struct _CHANNEL
{
  char		compCode[TRACE2_CHAN_LEN];	/* Component code.	*/
  char		netCode[TRACE2_NET_LEN];	/* Network code.	*/
  char		staCode[TRACE2_STA_LEN];	/* Station code (name).	*/
  char		locCode[TRACE2_LOC_LEN];	/* Location code	*/
} CHANNEL;

/*	Information about a particular subnet				*/
typedef struct _SUBNET
{
  int	stations[NSUBSTA];	/* stations in this subnet, by station 	*/
				/*   array index 			*/
  int	nStas;			/* number of stations in this subnet	*/
  int	minToTrigger;		/* Minimum number of stations needed to	*/
				/*   cause this subnet to trigger.	*/
  int	Triggered;		/* subnet trigger status: 		*/
				/*   1: triggered			*/
				/*   0: not triggered			*/
  /* Changed by Eugene Lublinsky, 3/31/Y2K */
  /* subnetCode expanded from 4 symbols to 7; triggerable added 	*/
  /* subnetCode changed to length MAX_SUBNET_LEN Carol 3/21/01 		*/
  char  subnetCode[MAX_SUBNET_LEN]; /* char string to identify this subnet  */
  int   triggerable[NSUBSTA];   /* a flag that is 1 if this station can trigger the subnet; 0 if not */

} SUBNET;

  /*	CarlSubTrig Network structure					*/
typedef struct _NETWORK
{
  CSUEWH	csuEwh;		/* Pointer to the Earthworm parameters.	*/
  CSUPARAM	csuParam;	/* Network parameters.			*/
  SHM_INFO	regionOut;      /* Output shared memory region info     */
  MSG_LOGO	hrtLogo;	/* Logo of outgoing heartbeat message.	*/
  MSG_LOGO	errLogo;	/* Logo of outgoing error message.	*/
  MSG_LOGO	trgLogo;	/* Logo of outgoing triglist message.	*/
  pid_t         MyPid;		/* For restart by startstop		*/
  STATION*	stations;	/* Array of stations in the network	*/
  SUBNET*       subnets;	/* Array of subnets in the network	*/
  CHANNEL*      channels;       /* non-seismic channels                 */
  mutex_t	stationMutex;	/* Lock for station trigger queues	*/
  int		nSta;		/* Number of stations in the net	*/
  int		nSub;		/* Number of subnets in the net		*/
  int		nSlots;		/* Number of pending station trigger	*/
  				/*   slots				*/
  int           nChan;          /* Number of extra channels             */
  int           numSub;         /* Max number of subnets that triggered */
                                /*   at the same time                   */
  long		PreEventTime;	/* Event start time adjustment		*/
				/*   (seconds).				*/
  long  	NetTrigDur;	/* base network trigger duration	*/
				/*   (seconds).				*/
  long  	subnetContrib;	/* Subnet contribution to trigger 	*/
				/*   duration (seconds)			*/
  long  	DefaultStationDur;	/* How long a station trigger	*/
  				/*   lasts if the trigger_off message	*/
  				/*   never arrives. (seconds)		*/
  long		MaxDur;		/* Maximum duration of network trigger	*/
  long		duration;	/* How long this triggered has lasted	*/
  time_t	onTime;		/* Time the network triggered		*/  
  long		latency;	/* Number of seconds Network runs 	*/
				/*   behind real-time.			*/
  int           useDataTime;    /* flag not to use system time, but data*/
  long		eventID;	/* ID of the last event detected.	*/
  int           numSubAll;      /* If this many subnets trigger, put    */
                                /*   wildcards in the trigger message   */
  int           listSubnets;    /* Which stations to put in triglist msg*/
                                /*  0: list all triggered stations      */
                                /*  1: list all stations in triggered   */ 
                                /*     subnets                          */
                                /*  2: list all stations in triggered   */
                                /*     subnets plus any other triggered */
                                /*     stations (0+1)                   */
                                /*  3: list all stations in subnets that*/ 
                                /*     had any stations triggered       */
  int           compAsWild;     /* Flag to replace component name with  */
                                /*   the wildcard in trigger message    */
  int		terminate;	/* Flag to terminate the subnet thread	*/  
  char*		trigMsgBuf;	/* Buffer for the trigger message	*/
  size_t	trigMsgBufLen;	/* Size of the trigger message buffer	*/
  int           coincident_stas; /* ignore a subnet trigger of more than this number of stations have the same onset time; 0=off */ 
  int           ignore_coincident; /* ignore a coincident check if more than this number of subnets fire */ 
  int           early_warning;  /* RSL: Launch message when net trigger is on */
} NETWORK;

/*	All the CarlStaTrig function prototypes				*/
void AddExtTrig( NETWORK* csuNet );
int CompareSCNs( const void *s1, const void *s2 );
STATION* FindStation( char* staCode, char* compCode, char* netCode,
		      char* locCode, NETWORK* csuNet );
long GetSubnet( NETWORK *csuNet );
int InitializeParameters( NETWORK* csuNet );
int InitializeStation( STATION* station );
void InitializeSubnet( SUBNET* subnet, int nSubs );
int IsComment( char string[] );
int ProduceTriggerMessage( NETWORK* csuNet);
int ProcessStationTrigger( NETWORK* csuNet, char* msg );
int ReadChannels( NETWORK* csuNet, char* chanStr );
int ReadConfig( char* filename, NETWORK* csuNet );
int ReadEWH( NETWORK* csuNet );
int ReadStations( NETWORK* csuNet );
int ReadSubnets( NETWORK* csuNet );
void RemoveStaTrig( STATION* sta, STATRIG* trig, int nSlots );
thr_ret SubnetThread( void* net );
void StatusReport( NETWORK* csuNet, unsigned char type, short code, 
		   char* message );

#endif /*	__CARLSUBTRIG_H__				   	*/
