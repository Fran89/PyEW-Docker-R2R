
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: compress_UA.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.9  2004/04/16 16:35:18  dietz
 *     cleaned out unused variables
 *
 *     Revision 1.8  2004/04/15 23:37:21  dietz
 *     modified to work with location codes
 *
 *     Revision 1.7  2002/08/13 16:39:15  dietz
 *     Removed hard-coded limit on the number of GetWavesFrom command
 *
 *     Revision 1.6  2002/06/19 21:13:57  dietz
 *     Initialized UseOriginalLogo to zero
 *
 *     Revision 1.5  2002/06/19 18:32:52  dietz
 *     Added optional config command, UseOriginalLogo, to allow user to
 *     apply the installation id & module id of the original uncompressed
 *     msg to the outgoing compressed message.
 *
 *     Revision 1.4  2002/06/05 15:36:58  patton
 *     Made Logit changes.
 *
 *     Revision 1.3  2001/05/09 18:21:25  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2000/07/24 20:38:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 16:22:40  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:22:08  lucky
 *     Initial revision
 *
 *
 */

/*
 * compress_UA.c:  
 *
 *  Implements compression routine supplied by University of Alaska 
 *  to compress wave data (TYPE_TRACEBUF) coming from the InRing defined 
 *  in compress_UA.d into data of type TYPE_TRACE_COMP_UA which is put to 
 *  the OutRing
 *  
 *  Gencompress algorithm used with permission from Boulder Real Time Technologies, Inc. 
 *  Copyright (c) 1997
 *
 *  Initial version:
 *  Lucky Vidmar (lucky@Colorado.EDU) - Tue Mar 24 11:49:49 MST 1998
 *
 *  Serious memory leak patched, code made more efficient in many places.
 *     Lynn Dietz (dietz@usgs.gov) Oct 2 1998
 *
 *  Wed Nov 11 16:43:14 MST 1998 lucky
 *   Part of Y2K compliance project:
 *    1) name of the config file passed to logit_init()
 *    2) incomming transport ring flushed at startup
 *    3) Process ID sent on with the heartbeat msgs for restart
 *       purposes.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>


/* Functions in this source file 
 *******************************/
static	int   compress_config (char *);
static	int   compress_lookup (void);
static	void  compress_status (unsigned char, short, char *);
static	int   brtt_gencompress (unsigned char **, int *, int *, int *, int, int);
static	void  bitpack (unsigned char *, int *, int *, int, int);
static 	int   check_scnl_logo (MSG_LOGO *, TracePacket *, int *);

static  SHM_INFO  InRegion;      /* shared memory region to use for i/o    */
static  SHM_INFO  OutRegion;     /* shared memory region to use for i/o    */

MSG_LOGO *GetLogo;               /* array for requesting instid,module,type */
short     nLogo=0;
 
/* Combination of the SCNL codes 
 *******************************/
typedef struct scnl {
   char   sta[TRACE2_STA_LEN];    /* Site name */
   char   chan[TRACE2_CHAN_LEN];  /* Component/channel code */
   char   net[TRACE2_NET_LEN];    /* Network name */
   char   loc[TRACE2_LOC_LEN];    /* location code */
   char   wildsta;                /* 1 if sta is wildcard;  0 otherwise */
   char   wildchan;               /* 1 if chan is wildcard; 0 otherwise */
   char   wildnet;                /* 1 if net is wildcard;  0 otherwise */
   char   wildloc;                /* 1 if loc is wildcard;  0 otherwise */
} SCNL;

#define  INCREMENT_SCNL  5;
int      MaxSCNL = 0;
SCNL    *GetSCNL = NULL;   /* array for requesting scnl,instid,module */
short    nSCNL   = 0;
        
/* Things to read or derive from configuration file
 **************************************************/
#define        NUM_COMMANDS  7      /* how many required commands in the config file */
static char    InRingName[MAX_RING_STR];      /* name of transport ring for i/o    */
static char    OutRingName[MAX_RING_STR];     /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
static char    MyProgName[256];     /* this module's program name */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */
static int     UseOriginalLogo=0;   /* flag for outgoing logo on compressed data */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static long          OutKey;        /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeTraceBuf;
static unsigned char TypeTraceBuf2;
static unsigned char TypeComp;
static unsigned char TypeComp2;
static unsigned char TypeError;
static unsigned char InstWild;
static unsigned char ModWild;

/* Error messages used by compress 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_COMPRESS      3   /* compression routine failed */

static char  errText[256];    /* string for log/error messages */


/* Defines needed by the compression code
 ****************************************/
#define	ABS(x)	(((x)>=0)?(x):(-x))

int gc_pbit[31] = { 1, 2, 4, 8, 16, 32, 64, 128,
		256, 512, 1024, 2048, 4096,
		8192, 16384, 32768, 65536,
		131072, 262144, 524288, 1048576,
		2097152, 4194304, 8388608, 16777216,
		33554432, 67108864, 134217728,
		268435456, 536870912, 1073741824 };

#define	LENGTH	        25


pid_t MyPid;    /** Hold our process ID to be sent with heartbeats **/

int isWild( char *str )
{
  if( (strcmp(str,"*")==0)  ) return 1;
  return 0;
}

int notMatch( char *str1, char *str2 )
{
  return( strcmp(str1,str2) );
}

int main (int argc, char **argv)
{
   int            res;            /* tmp result of function calls  */
   int            i;              /* index counter                 */
   int            gotit;
   time_t         timeNow;        /* current time                  */ 
   time_t         timeLastBeat;   /* time last heartbeat was sent  */
   long           recsize;        /* size of retrieved message     */
   MSG_LOGO       reclogo;        /* logo of retrieved message     */
   MSG_LOGO       logo;           /* logo of outgoing message      */

   TracePacket    WaveBuf;        /* variable to hold wave message */
   int32_t	 *WaveLong;       /* if wave data is longs         */
   short         *WaveShort;      /* if wave data is shorts        */

   TracePacket    CompBuf;        /* to hold compressed data buffer */
   unsigned char *CompData;       /* compressed data content        */
   int            CompBufLen;     /* actual length of compressed pkt*/

   unsigned char *compress_out;   /* holds output of gencompress   */
   int            compress_nout;  /* number of compressed data     */
   int            compress_size;  /* size of compressed output     */
   int            compress_in[MAX_TRACEBUF_SIZ]; 
                                  /* holds input to gencompress    */
   int            exitstatus = EW_FAILURE;


/* Check command line arguments 
 ******************************/
   if (argc != 2)
   {
      fprintf (stderr, "Usage: compress_UA <configfile>\n");
      return EW_FAILURE;
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init (argv[1], 0, 256, 1);

/* To be used in logging functions
 *********************************/
   if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
   {
      logit( "e", "compress_UA: Call to get_prog_name failed.\n");
      return EW_FAILURE;
   }

/* Read the configuration file(s)
 ********************************/
   if (compress_config (argv[1]) != EW_SUCCESS)
   {
      logit( "e", "compress_UA: Call to compress_config failed\n");
      return EW_FAILURE;
   }
   logit ("", "%s(%s): Read command file <%s>\n", 
           MyProgName, MyModName, argv[1]);

/* Look up important info from earthworm.h tables
 ************************************************/
   if (compress_lookup () != EW_SUCCESS)
   {
      logit( "e", "compress_UA: Call to compress_lookup failed\n");
      return EW_FAILURE;
   }

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init (argv[1], 0, 256, LogSwitch);

/* Attach to Input shared memory ring 
 ************************************/
   tport_attach (&InRegion, InKey);
   logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
           MyProgName, MyModName, InRingName, InKey);

/* Attach to Output shared memory ring 
 *************************************/
   tport_attach (&OutRegion, OutKey);
   logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
          MyProgName, MyModName, OutRingName, OutKey);

/* Get our process ID
 **********************/
   if ((MyPid = getpid ()) == -1)
   {
      logit ("e", "%s(%s): Call to getpid failed. Exiting.\n",
              MyProgName, MyModName);
      return (EW_FAILURE);
   }

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

/* Point to header and data portions of waveform/compressed message
 ******************************************************************/
   WaveLong  = (int32_t *)  (WaveBuf.msg + sizeof(TRACE2_HEADER));
   WaveShort = (short *) (WaveBuf.msg + sizeof(TRACE2_HEADER));
   CompData  = (unsigned char *) (CompBuf.msg + sizeof(TRACE2_HEADER));

/* Initialize variables used inside the brtt_gencompress() routine. 
 ******************************************************************/
   compress_out  = NULL;  /* address if output array         */
   compress_size = 0;     /* length (bytes) of output array  */
   compress_nout = 0;     /* length (data samples) of output */

/* Set outgoing msg logo
 ***********************/
   logo.instid = InstId;
   logo.mod    = MyModId;

/* Flush the incoming transport ring
 ***********************************/
   while (tport_getmsg (&InRegion, GetLogo, nLogo,
          &reclogo, &recsize, WaveBuf.msg, MAX_TRACEBUF_SIZ) != GET_NONE);

 
/*------------------- setup done; start main loop ---------------------*/

   while (1)
   {
   /* send compress' heartbeat
    ***************************/
      if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
      {
         timeLastBeat = timeNow;
         compress_status (TypeHeartBeat, 0, ""); 
      }

   /* Process all new messages    
    **************************/
      do
      {
      /* see if a termination has been requested 
       *****************************************/
         if (tport_getflag (&InRegion) == TERMINATE ||
             tport_getflag (&InRegion) == MyPid )
         {
            exitstatus = EW_SUCCESS;
            logit ("t", "%s(%s): Termination requested; exiting!\n",
                   MyProgName, MyModName);
            goto ShutDown;
         }

      /* Get msg & check the return code from transport
       ************************************************/
         res = tport_getmsg (&InRegion, GetLogo, nLogo,
                             &reclogo, &recsize, WaveBuf.msg, MAX_TRACEBUF_SIZ);

         if (res == GET_NONE)  break;  /* no more new messages     */
				
         else if (res == GET_TOOBIG)   /* next message was too big */
         {                             /* complain and try again   */
            sprintf (errText, 
                     "Retrieved msg[%ld] (i%u m%u t%u) too big for WaveBuf[%d]",
                      recsize, reclogo.instid, reclogo.mod, reclogo.type, 
                      MAX_TRACEBUF_SIZ);
            compress_status (TypeError, ERR_TOOBIG, errText);
            continue;
         }
         else if (res == GET_MISS)     /* got a msg, but missed some */
         {
            sprintf (errText, "Missed msg(s)  i%u m%u t%u  %s.",
                     reclogo.instid, reclogo.mod, reclogo.type, InRingName );
            compress_status (TypeError, ERR_MISSMSG, errText);
         }
         else if (res == GET_NOTRACK)  /* got a msg, but can't tell */
         {                             /* if any were missed        */
            sprintf (errText,
                     "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                      reclogo.instid, reclogo.mod, reclogo.type);
            compress_status (TypeError, ERR_NOTRACK, errText);
         }

      /* If necessary, swap bytes in the wave message
       **********************************************/
         if (WaveMsg2MakeLocal( &(WaveBuf.trh2) ) < 0)
         {
            logit ("et", "%s(%s): Unknown datatype <%s>.\n",
		    MyProgName, MyModName, WaveBuf.trh2.datatype );
            continue;
         }

      /* Check to see if msg's SCNL and Logo are desired
       ************************************************/
         if (check_scnl_logo (&reclogo, &WaveBuf, &gotit) != EW_SUCCESS)
         {
            logit ("et", "%s(%s): Call to check_scnl_logo failed; exiting.\n",
                     MyProgName, MyModName);
            goto ShutDown;
         }

      /* If the message matches a desired SCNL, compress it 
       *****************************************************/
         if (gotit == TRUE)
         {
            compress_nout = 0;

         /* Copy wave data by casting appropriately 
          *****************************************/
            if (WaveBuf.trh2.datatype[1] == '2' )
            {
               for (i = 0; i < WaveBuf.trh2.nsamp; i++) 
                  compress_in[i] = (int) WaveShort[i];
            }
            else if (WaveBuf.trh2.datatype[1] == '4' )
            {
               for (i = 0; i < WaveBuf.trh2.nsamp; i++) 
                  compress_in[i] = (int) WaveLong[i];
            }
            else
            {
               logit("e","Unknown datatype <%s> in tracebuf header\n",
                      WaveBuf.trh2.datatype );
               continue;
            }

            if (brtt_gencompress (&compress_out, &compress_nout, 
                             &compress_size, compress_in, 
                             WaveBuf.trh2.nsamp, 0) != EW_SUCCESS )
            {                             
               compress_status (TypeError, ERR_COMPRESS, 
                                "brtt_gencompress failed in allocation");
               continue;
            }
		
         /* Copy the headers of the wave to the compressed message
          * DO NOT change anything (as per Alex, 4/27/98 )
          *********************************************************/
            memcpy( CompBuf.msg, WaveBuf.msg, sizeof(TRACE2_HEADER) );
	
         /* Copy in the compressed data after the header
          **********************************************/
	    memcpy( CompData, compress_out, (size_t)compress_nout );

         /* Set new (real) length of the output message 
          ***********************************************/
            CompBufLen = (compress_nout * sizeof (unsigned char)) + 
                          sizeof (TRACE2_HEADER);

         /* Set logo and put compressed message onto the output ring
          **********************************************************/
            if( reclogo.type==TypeTraceBuf2 ) logo.type=TypeComp2;
            else                              logo.type=TypeComp;

            if (UseOriginalLogo) 
            {
               logo.instid = reclogo.instid;
               logo.mod    = reclogo.mod;
            }
            if (tport_putmsg (&OutRegion, &logo, 
                              CompBufLen, CompBuf.msg ) != PUT_OK)
            {
               logit ("et", "%s(%s):  Error sending message: i%d m%d t%d.\n", 
                       MyProgName, MyModName,
                       logo.instid, logo.mod, logo.type);
            }
	
         } /* Is this a right message - SCNL */

      } while(res != GET_NONE);  /* end of message-processing-loop */
   	     
      sleep_ew (20);  /* no more messages; wait for new ones to arrive */
	
   } /* while (1) */  

ShutDown:
 
/* Free malloc'd memory
 **********************/
   free (compress_out);
   free (GetLogo);
   free (GetSCNL);
          
/* Detach from shared memory
 ***************************/
   tport_detach (&InRegion);
   tport_detach (&OutRegion);
   fflush (stdout);
   return (exitstatus);
}

/******************************************************************************
 *  compress_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int compress_config (char *configfile)
{
	char     init[NUM_COMMANDS]; /* init flags, 1 for each required command */
	int      nmiss;              /* # of required commands that were missed */
	char    *com;
	char    *str;
	int      nfiles;
	int      success;
	int      i;

	/* Set to zero one init flag for each required command 
	*****************************************************/   
	for (i = 0; i < NUM_COMMANDS; i++) init[i] = 0;

	/* Open the main configuration file 
	**********************************/
	nfiles = k_open (configfile); 
	if (nfiles == 0) 
	{
		logit ("e",
			"compress_UA: Error opening command file <%s>; exiting!\n", 
				configfile);
		return EW_FAILURE;
	}

	/* Process all command files
	***************************/
	while (nfiles > 0)   /* While there are command files open */
	{
		while (k_rd ())        /* Read next line from active file  */
		{  
			com = k_str ();         /* Get the first token from line */

			/* Ignore blank lines & comments
			*******************************/
			if (!com)
				continue;
			if (com[0] == '#')
				continue;

			/* Open a nested configuration file 
			**********************************/
			if (com[0] == '@') 
			{
				success = nfiles + 1;
				nfiles  = k_open (&com[1]);
				if (nfiles != success) 
				{
					logit ("e", 
				  	       "compress_UA: Error opening command file <%s>; "
                                               "exiting!\n", &com[1]);
					return EW_FAILURE;
				}
				continue;
			}

			/* Process anything else as a command 
			************************************/
			if (k_its ("MyModId")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (MyModName, str);
					init[0] = 1;
				}
			}
			else if (k_its ("InRing")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (InRingName, str);
					init[1] = 1;
				}
			}
			else if (k_its ("OutRing")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (OutRingName, str);
					init[2] = 1;
				}
			}
			else if (k_its ("HeartBeatInterval")) 
			{
				HeartBeatInterval = k_long ();
				init[3] = 1;
			}


			/* Enter SCNL of wave msgs to process
		         *************************************/
			else if (k_its ("CompressSCNL")) 
			{
				if (nSCNL >= MaxSCNL) 
				{
                                    size_t  size;
                                    MaxSCNL += INCREMENT_SCNL;
                                    size     = MaxSCNL * sizeof( SCNL );
                                    GetSCNL  = (SCNL *) realloc( GetSCNL, size );
                                    if( GetSCNL == NULL )
                                    {
                                       logit( "e",
                                              "compress_UA: Error allocating %d bytes"
                                              " for <CompressSCNL> list; exiting!\n", size );
                                       return EW_FAILURE;
                                    }
				}
                                str = k_str();
                                if( !str || strlen(str) >= (size_t)TRACE2_STA_LEN ) {
                                       logit( "e",
                                              "compress_UA: Bad station code in"
                                              " <CompressSCNL> cmd; exiting!\n" );
                                       return EW_FAILURE;
				}
                	        strcpy(GetSCNL[nSCNL].sta, str);
                                GetSCNL[nSCNL].wildsta = (char)isWild( str );

                                str = k_str();
                                if( !str || strlen(str) >= (size_t)TRACE2_CHAN_LEN ) {
                                       logit( "e",
                                              "compress_UA: Bad component code in"
                                              " <CompressSCNL> cmd; exiting!\n" );
                                       return EW_FAILURE;
				}
                	        strcpy(GetSCNL[nSCNL].chan, str);
                                GetSCNL[nSCNL].wildchan = (char)isWild( str );

                                str = k_str();
                                if( !str || strlen(str) >= (size_t)TRACE2_NET_LEN ) {
                                       logit( "e",
                                              "compress_UA: Bad network code in"
                                              " <CompressSCNL> cmd; exiting!\n" );
                                       return EW_FAILURE;
				}
                	        strcpy(GetSCNL[nSCNL].net, str);
                                GetSCNL[nSCNL].wildnet = (char)isWild( str );

                                str = k_str();
                                if( !str || strlen(str) >= (size_t)TRACE2_LOC_LEN ) {
                                       logit( "e",
                                              "compress_UA: Bad location code in"
                                              " <CompressSCNL> cmd; exiting!\n" );
                                       return EW_FAILURE;
				}
                	        strcpy(GetSCNL[nSCNL].loc, str);
                                GetSCNL[nSCNL].wildloc = (char)isWild( str );

				nSCNL   = nSCNL + 1;
				init[4] = 1;

			} /* CompressSCNL */

			else if (k_its ("LogFile"))
			{
				LogSwitch = k_int();
				init[5] = 1;
			}

			else if ( k_its( "GetLogo" ) )  
			{
				MSG_LOGO *tlogo = NULL;
				tlogo = (MSG_LOGO *)realloc( GetLogo, (nLogo+1)*sizeof(MSG_LOGO) );
				if( tlogo == NULL )
				{
				   logit( "e", 
                                          "compress_UA: GetLogo: error reallocing"
                                          " %d bytes; exiting!\n",
                                          (nLogo+1)*sizeof(MSG_LOGO) );
                                   return EW_FAILURE;
				}
                                GetLogo = tlogo;

				if( ( str = k_str() ) != NULL )
				{
				   if ( GetInst( str, &(GetLogo[nLogo].instid) ) != 0 )
				   { 
				      logit( "e",
				             "compress_UA: GetLogo: invalid installation `%s'"
				             " in `%s'; exiting!\n", str, configfile );
				      return EW_FAILURE;
				   } 
				}   
				if( ( str = k_str() ) != NULL )
				{
				   if ( GetModId( str, &(GetLogo[nLogo].mod) ) != 0 )
				   { 
				      logit( "e",
				             "compress_UA: GetLogo: invalid module id `%s'"
				             " in `%s'; exiting!\n", str, configfile );
				      return EW_FAILURE;
				   } 
				}   
				if( ( str = k_str() ) != NULL )
				{
				   if ( strcmp( str, "TYPE_TRACEBUF2" )!=0 &&
				        strcmp( str, "TYPE_TRACEBUF"  )!=0    )
				   { 
				      logit( "e",
				             "compress_UA: GetLogo: invalid msgtype `%s'"
				             " in `%s'; must be either TYPE_TRACEBUF or"
				             " TYPE_TRACEBUF2; exiting!\n", str, configfile );
				      return EW_FAILURE;
				   } 
    				   if ( GetType( str, &(GetLogo[nLogo].type) ) != 0 )
				   { 
				      logit( "e",
				             "compress_UA: GetLogo: invalid msgtype `%s'"
				             " in `%s'; exiting!\n", str, configfile );
				      return EW_FAILURE;
				   } 
				}   
				nLogo++;
				init[6] = 1;
			} /* end GetLogo */

        /*optional*/    else if (k_its ("UseOriginalLogo"))
                        {
                                UseOriginalLogo = k_int();
                        }

			/* Unknown command
			*****************/ 
			else 
			{
				logit ("e", "compress_UA: <%s> Unknown command in <%s>.\n", 
								com, configfile);
				continue;
			}

			/* See if there were any errors processing the command 
			*****************************************************/
			if (k_err ()) 
			{
				logit ("e", 
					"compress_UA: Bad <%s> command in <%s>; exiting!\n",
						com, configfile);
				return EW_FAILURE;
			}

		} /** while k_rd() **/

		nfiles = k_close ();

	} /** while nfiles **/

	/* After all files are closed, check init flags for missed commands
	******************************************************************/
	nmiss = 0;
	for(i = 0; i < NUM_COMMANDS; i++)  if(!init[i])  nmiss++;

	if (nmiss) 
	{
		logit ("e", "compress_UA: ERROR, no ");
		if (!init[0])  logit ("e", "<MyModId> "           );
		if (!init[1])  logit ("e", "<InRing> "            );
		if (!init[2])  logit ("e", "<OutRing> "           );
		if (!init[3])  logit ("e", "<HeartBeatInterval> " );
		if (!init[4])  logit ("e", "<CompressSCNL> "      );
        	if (!init[5])  logit ("e", "<LogFile> "           );
        	if (!init[6])  logit ("e", "<GetLogo> "           );
		logit ("e", "command(s) in <%s>; exiting!\n", configfile);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/******************************************************************************
 *  compress_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int compress_lookup( void )
{

	/* Look up keys to shared memory regions
	*************************************/
	if ((InKey = GetKey (InRingName)) == -1) 
	{
		logit( "e",
			"compress_UA:  Invalid ring name <%s>; exiting!\n", InRingName);
		return EW_FAILURE;
	}

	if (( OutKey = GetKey (OutRingName)) == -1) 
	{
		logit( "e",     
			"compress_UA:  Invalid ring name <%s>; exiting!\n", OutRingName);
		return EW_FAILURE;
	}

	/* Look up installations of interest
	*********************************/
	if (GetLocalInst (&InstId) != 0) 
	{
		logit( "e",      
			"compress_UA: error getting local installation id; exiting!\n");
		return EW_FAILURE;
	}


	if (GetInst ("INST_WILDCARD", &InstWild) != 0) 
	{
		logit( "e",      
			"compress_UA: error getting wildcard installation id; exiting!\n");
		return EW_FAILURE;
	}

	/* Look up modules of interest
	***************************/
	if (GetModId (MyModName, &MyModId) != 0) 
	{
		logit( "e",      
		  "compress_UA: Invalid module name <%s>; exiting!\n", MyModName);
		return EW_FAILURE;
	}

	if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
	{
		logit( "e",      
		       "compress_UA: Invalid module name <MOD_WILDCARD>; exiting!\n" ); 
		return EW_FAILURE;
	}

	/* Look up message types of interest
	*********************************/
	if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
	{
		logit( "e",      
		  "compress_UA: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_ERROR", &TypeError) != 0) 
	{
		logit( "e",      
		  "compress_UA: Invalid message type <TYPE_ERROR>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_TRACEBUF", &TypeTraceBuf) != 0) 
	{
		logit ("e",
			"compress_UA: Invalid message type <TYPE_TRACEBUF>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_TRACEBUF2", &TypeTraceBuf2) != 0) 
	{
		logit ("e",
			"compress_UA: Invalid message type <TYPE_TRACEBUF2>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_TRACE_COMP_UA", &TypeComp) != 0) 
	{
		logit( "e",     
		  "compress_UA: Invalid message type <TYPE_TRACE_COMP_UA>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_TRACE2_COMP_UA", &TypeComp2) != 0) 
	{
		logit( "e",     
		  "compress_UA: Invalid message type <TYPE_TRACE2_COMP_UA>; exiting!\n");
		return EW_FAILURE;
	}

	return EW_SUCCESS;

} 

/******************************************************************************
 * compress_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void compress_status( unsigned char type, short ierr, char *note )
{

	MSG_LOGO    logo;
	char        msg[256];
	long        size;
	time_t        t;

	/* Build the message
	*******************/ 
	logo.instid = InstId;
	logo.mod    = MyModId;
	logo.type   = type;

	time (&t);

	if (type == TypeHeartBeat)
	{
		sprintf (msg, "%ld %ld\n", (long) t, (long) MyPid);
	}
	else if (type == TypeError)
	{
		sprintf (msg, "%ld %hd %s\n", (long) t, ierr, note);
		logit ("et", "compress_UA: %s\n", note);
	}

	size = (long)strlen (msg);   /* don't include the null byte in the message */

	/* Write the message to shared memory
	************************************/
	if (tport_putmsg (&OutRegion, &logo, size, msg) != PUT_OK)
	{
		if (type == TypeHeartBeat) 
		{
		   logit ("et", "compress_UA:  Error sending heartbeat.\n");
		}
		else if (type == TypeError) 
		{
		   logit ("et", "compress_UA:  Error sending error:%d.\n", ierr);
		}
	}

}


/******************************************************************************
 * brtt_gencompress () - generic compression routine; takes an input          *
 *                  integer data array and compresses it into an output       *
 *                  byte array                                                *
 *                                                                            *
 *                                                                            *
 *    Copyright (c) 1997 Boulder Real Time Technologies, Inc.                 *
 *                                                                            *
 ******************************************************************************/
static int brtt_gencompress ( unsigned char **out, int *nout, int *size, 
			      int *in, int nsamp, int length )
{
	int n = 0;
	int samp = 0;
	int nbits = 0;
	int i = 0, j = 0, k = 0, l = 0;
	int msbit = 0;
	unsigned char *bout;


	if (length <= 0) 
		length = LENGTH;
	if (length > 256) 
		length = 256;
	if (*out == NULL) 
	{
		*size = 2*nsamp;
		*out = (unsigned char *) malloc (*size);
		if (*out == NULL) 
		{
			logit( "e", "brtt_gencompress: malloc() error.\n");
			return EW_FAILURE;
		}
	}

	bout = *out;
	*((int *)bout) = htonl(nsamp);
	n = 4;
	bout[n] = (unsigned char)length;
	n++;

	for (i = 0, j = 0; j < nsamp; i++) 
	{
		nbits = 0;

		for (k = 0; k < length; k++) 
		{
			if (j + k >= nsamp) 
				break;

			samp = ABS(in[j+k]);
			for (l = 0; l < 31; l++) 
			{
				if (samp < gc_pbit[l]) 
					break;
			}
			if (l > nbits) 
				nbits = l;
		}

		bout[n] = (unsigned char)nbits;
		n++;
		while (n+8 >= *size) 
		{
			*size += 50;
			*out = (unsigned char *) realloc (*out, *size);
			if (*out == NULL) 
			{
				logit( "e", "brtt_gencompress: realloc() error.\n");
				return EW_FAILURE;
			}
			bout = *out;
		}

		msbit=7;
		for (k = 0; k < length; k++, j++) 
		{
			if (j >= nsamp) 
				break;

			bitpack (bout, &n, &msbit, nbits+1, in[j]);

			while (n+8 >= *size) 
			{
				*size += 50;
				*out = (unsigned char *) realloc (*out, *size);
				if (*out == NULL) 
				{
					logit( "e", "brtt_gencompress: realloc() error.\n");
					return EW_FAILURE;
				}
				bout = *out;
			}
		}


		if (msbit != 7) 
			n++;
		if (j >= nsamp) 
			break;
		while (n+8 >= *size) 
		{
			*size += 50;
			*out = (unsigned char *) realloc (*out, *size);
			if (*out == NULL) 
			{
				logit( "e", "brtt_gencompress: realloc() error.\n");
				return EW_FAILURE;
			}
			bout = *out;
		}
	}

	*nout = n;

	return EW_SUCCESS;
}



/******************************************************************************
 * bitpack () - part of the compression routine                               *
 *                                                                            *
 *    Copyright (c) 1997 Boulder Real Time Technologies, Inc.                 *
 *                                                                            *
 ******************************************************************************/
static void bitpack (unsigned char *out, int *n, int *msbit, 
		    int length, int in)
{
	int lsbit=0;
	unsigned char mask=0, omask=0;
	int nn=0;

	lsbit = *msbit - length + 1;
	if (lsbit < 0) 
	{
		nn = *msbit + 1;
		mask = (1<<nn)-1;
		out[*n] = (out[*n] & (~mask)) | ((in>>(-lsbit)) & mask);
		(*n)++;
		*msbit = 7;
		bitpack (out, n, msbit, length-nn, in);
	} 
	else 
	{
		mask = (1<<length)-1;
		if (lsbit == 0) 
		{
			out[*n] = (out[*n] & (~mask)) | (in & mask);
			(*n)++;
			*msbit = 7;
		} 
		else 
		{
			omask = mask<<lsbit;
			out[*n] = (out[*n] & (~omask)) | ((in<<lsbit) & omask);
			(*msbit) -= length;
		}
	}
}

/********************************************************************
 * check_scnl_logo () - check message against desired SCNLs         *
 ********************************************************************/
static int check_scnl_logo(MSG_LOGO *msglogo, TracePacket *pkt, int *gotit )
{
   int       i;

   if ((msglogo == NULL) || (pkt == NULL))
   {
      logit ("et",  "compress_UA: invalid parameters to check_scnl_logo\n");
      return(EW_FAILURE);
   }

   *gotit = FALSE;  /* assume it's not a match */

/* Look for match in list of requested SCNL (remember wildcards)
 ***************************************************************/
   for( i=0; i<nSCNL; i++ )
   {
      if( !GetSCNL[i].wildsta  && notMatch(GetSCNL[i].sta,  pkt->trh2.sta)  ) continue;
      if( !GetSCNL[i].wildchan && notMatch(GetSCNL[i].chan, pkt->trh2.chan) ) continue;
      if( !GetSCNL[i].wildnet  && notMatch(GetSCNL[i].net,  pkt->trh2.net)  ) continue;
      if(  msglogo->type==TypeTraceBuf2 &&
          !GetSCNL[i].wildloc  && notMatch(GetSCNL[i].loc,  pkt->trh2.loc)  ) continue;
     *gotit = TRUE;
      return(EW_SUCCESS);
   }
   return(EW_SUCCESS);
}

