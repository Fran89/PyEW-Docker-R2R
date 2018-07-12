
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: decompress_UA.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2007/03/28 18:29:36  paulf
 *     removed malloc.h since it is now in platform.h and not used on some platforms
 *
 *     Revision 1.8  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.7  2004/04/16 18:55:32  dietz
 *     modified to work with TYPE_TRACE2_COMP_UA input
 *
 *     Revision 1.6  2002/08/14 00:30:09  dietz
 *     Syntax change in brtt_genuncompress to avoid infinite loop
 *     on realloc.
 *
 *     Revision 1.5  2002/06/19 18:07:08  dietz
 *     Added optional config command, UseOriginalLogo, to allow user to
 *     apply the installation id & module id of the original compressed
 *     msg to the outgoing decompressed message.
 *
 *     Revision 1.4  2002/06/05 15:00:46  patton
 *     Made Logit changes.
 *
 *     Revision 1.3  2001/05/09 18:34:48  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2000/07/24 20:38:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 16:58:14  lucky
 *     Initial revision
 *
 *
 */

/*
 *  decompress_UA.c:
 *
 *  Implements decompression routine from U. of A. to decompress data
 *  of the type TYPE_TRACE_COMP_UA or TYPE_TRACE2_COMP_UA coming from 
 *  the InRing defined decompress_UA.d into TYPE_TRACEBUF or TYPE_TRACEBUF2 
 *  which is put to the OutRing
 *
 *  Genuncompress algorithm used with permission from Boulder Real Time Technologies, Inc.
 *  Copyright (c) 1997
 *
 *  Initial version:
 *  Lucky Vidmar (lucky@Colorado.EDU) - Tue Mar 24 11:49:49 MST 1998
 *
 *  Serious memory leak patched, code made more efficient in many places.
 *     Lynn Dietz (dietz@usgs.gov) Oct 1 1998
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
#include <time.h>
#include <sys/types.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>

/* Functions in this source file
 *******************************/
static	int   decompress_config (char *);
static	int   decompress_lookup (void);
static	void  decompress_status (unsigned char, short, char *);
static	int   brtt_genuncompress (int **, int *, int *, unsigned char *, int);
static  void  bitunpack (int *, int *, int *, int, unsigned char *);

#define   NUM_COMMANDS  6   /* how many required commands in the config file */

static  SHM_INFO  InRegion;    /* shared memory region to use for i/o    */
static  SHM_INFO  OutRegion;   /* shared memory region to use for i/o    */

MSG_LOGO *GetLogo;             /* array for requesting instid,module,type */
short     nLogo=0;

/* Things to read or derive from configuration file
 **************************************************/
static char    InRingName[MAX_RING_STR];  /* name of transport ring for i/o */
static char    OutRingName[MAX_RING_STR]; /* name of transport ring for i/o */
static char    MyModName[MAX_MOD_STR];    /* speak as this module name/id   */
static char    MyProgName[256];     /* this module's program name        */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */
static int     UseOriginalLogo=0;   /* flag for outgoing logo            */

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

/* Error messages used by decompress
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_UNCOMPRESS    3   /* call to uncompression routine failed   */

static char  errText[256];     /* string for log/error messages          */


/* Defines needed by the decompression code
 ******************************************/
#define	ABS(x)	(((x)>=0)?(x):(-x))

int gc_pbit[31] = { 1, 2, 4, 8, 16, 32, 64, 128,
		256, 512, 1024, 2048, 4096,
		8192, 16384, 32768, 65536,
		131072, 262144, 524288, 1048576,
		2097152, 4194304, 8388608, 16777216,
		33554432, 67108864, 134217728,
		268435456, 536870912, 1073741824 };

#define	LENGTH	25

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/



int main (int argc, char **argv)
{
   int		  i;	        /* index counter                 */
   int		  res;          /* tmp result of function calls  */
   time_t	  timeNow;      /* current time                  */
   time_t	  timeLastBeat; /* time last heartbeat was sent  */
   long		  recsize;      /* size of retrieved message     */
   MSG_LOGO 	  reclogo;      /* logo of retrieved message     */
   MSG_LOGO	  logo;         /* logo of outgoing message      */
   TracePacket    WaveBuf;      /* space to hold wave message    */
   int		  WaveBufLen;   /* length of WaveBuf             */
   short         *WaveShort;    /* if wave data is shorts        */
   int32_t       *WaveLong;     /* if wave data is int32s        */

   unsigned char *CompData;     /* compressed data content       */
   TracePacket    CompBuf;      /* space to hold comp message    */

   int	         *decompress_out;   /* holds output of genuncompress */
   int	          decompress_nout;  /* number of decompressed data   */
   int	          decompress_size;  /* size of decompressed output   */
   unsigned char  decompress_in[MAX_TRACEBUF_SIZ]; /* input to genuncompress */
   int	          inbyte;           /* # bytes in decompress_in      */

   int            exitstatus = EW_FAILURE;

/* Check command line arguments
 ******************************/
   if (argc != 2)
   {
      fprintf (stderr, "Usage: decompress_UA <configfile>\n");
      return EW_FAILURE;
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init(argv[1], 0, 256, 1 );
 
/* To be used in logging functions
 *********************************/
   if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
   {
      logit( "e", "decompress_UA: Call to get_prog_name failed.\n");
      return EW_FAILURE;
   }

/* Read the configuration file(s)
 ********************************/
   if (decompress_config (argv[1]) != EW_SUCCESS)
   {
      logit( "e", "decompress_UA: Call to decompress_config failed\n");
      return EW_FAILURE;
   }
   logit ("" , "%s: Read command file <%s>\n", MyProgName, argv[1]);

/* Look up important info from earthworm.h tables
 ************************************************/
   if (decompress_lookup () != EW_SUCCESS)
   {
      logit( "e", "decompress_UA: Call to decompress_lookup failed\n");
      return EW_FAILURE;
   }

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init(argv[1], 0, 256, LogSwitch );
   
/* Get our process ID
 **********************/
   if ((MyPid = getpid ()) == -1)
   {
      logit ("e", "%s(%s): Call to getpid failed. Exiting.\n",
              MyProgName, MyModName);
      return (EW_FAILURE);
   }

/* Attach to Input shared memory ring
 ************************************/
   tport_attach (&InRegion, InKey);
   logit ("", "decompress_UA: Attached to  input memory region %s: %d\n",
          InRingName, InKey);

/* Attach to Output shared memory ring
 *************************************/
   tport_attach (&OutRegion, OutKey);
   logit ("", "decompress_UA: Attached to output memory region %s: %d\n",
          OutRingName, OutKey);

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

/* Point to data portions of waveform/compressed message
 *******************************************************/
   WaveShort = (short *) (WaveBuf.msg + sizeof (TRACE2_HEADER));
   WaveLong  = (int32_t *)  (WaveBuf.msg + sizeof (TRACE2_HEADER));
   CompData  = (unsigned char *) (CompBuf.msg + sizeof (TRACE2_HEADER));

/* Initialize variables used inside the brtt_genuncompress() routine. */
   decompress_out  = NULL;  /* address if output array         */
   decompress_size = 0;     /* length (bytes) of output array  */
   decompress_nout = 0;     /* length (data samples) of output */

/* Set outgoing msg logo
 ***********************/
   logo.instid = InstId;
   logo.mod    = MyModId;

/* Flush the incoming transport ring
 ***********************************/
   while( tport_getmsg( &InRegion, GetLogo, nLogo,
          &reclogo, &recsize, CompBuf.msg, MAX_TRACEBUF_SIZ ) != GET_NONE);


/*--------------------- setup done; start main loop ------------------------*/

   while (1)
   {
   /* send decompress's heartbeat
    *****************************/
      if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval)
      {
         timeLastBeat = timeNow;
         decompress_status (TypeHeartBeat, 0, "");
      }

   /* Process all new messages
    **************************/
      do
      {
      /* Shut down if a termination has been requested
       ***********************************************/
         if (tport_getflag (&InRegion) == TERMINATE  ||
             tport_getflag (&InRegion) == MyPid )
         {
            exitstatus = EW_SUCCESS;
            logit ("t", "decompress_UA: Termination requested; exiting!\n");
            goto ShutDown;
         }

      /* Get msg & check the return code from transport
       ************************************************/
         res = tport_getmsg (&InRegion, GetLogo, nLogo,
                             &reclogo, &recsize, CompBuf.msg, MAX_TRACEBUF_SIZ);

         if (res == GET_NONE)  break;  /* no more new messages     */

         else if (res == GET_TOOBIG)   /* next message was too big */
         {                             /* complain and try again   */
            sprintf(errText,
                   "Retrieved msg[%ld] (i%u m%u t%u) too big for InBuf[%d]",
                    recsize, reclogo.instid, reclogo.mod, reclogo.type,
                    MAX_TRACEBUF_SIZ );
            decompress_status (TypeError, ERR_TOOBIG, errText);
            continue;
         }
         else if (res == GET_MISS)     /* got a msg, but missed some */
         {
            sprintf (errText, "Missed msg(s)  i%u m%u t%u  %s.",
                     reclogo.instid, reclogo.mod, reclogo.type, InRingName);
            decompress_status (TypeError, ERR_MISSMSG, errText);
         }
         else if (res == GET_NOTRACK)  /* got a msg, but can't tell */
         {                             /* if any were missed        */
            sprintf (errText,
                     "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                      reclogo.instid, reclogo.mod, reclogo.type);
            decompress_status (TypeError, ERR_NOTRACK, errText);
         }

      /* Got a message; make sure it's the kind we expect
       **************************************************/
         if (reclogo.type!=TypeComp2 &&
             reclogo.type!=TypeComp     )
         {
            logit("e", "decompress_UA: cannot process msg type: %d\n",
                   reclogo.type );
            continue;
         }

      /* Call decompression routine
       ****************************/
         decompress_nout = 0;

         /* Need to figure out how many data bytes are in msg  */
         inbyte = (int) (recsize - sizeof (TRACE2_HEADER)) /
                        (sizeof (unsigned char));

         memcpy( decompress_in, CompData, (size_t)inbyte );

         if (brtt_genuncompress (&decompress_out, &decompress_nout,
                            &decompress_size, decompress_in, inbyte) != EW_SUCCESS)
         {
            decompress_status (TypeError, ERR_UNCOMPRESS,
                               "brtt_genuncompress failed in allocation");
            continue; 
         }


      /* Build decompressed message
       ****************************/
         /* Start with a clean buffer */
         memset( (void *)WaveBuf.msg, 0, (size_t)MAX_TRACEBUF_SIZ );

         /* Copy the header of the compressed message;  */
         /* make sure it's in local byte order          */
         memcpy( WaveBuf.msg, CompBuf.msg, sizeof(TRACE2_HEADER) );
         if (WaveMsg2MakeLocal(&WaveBuf.trh2) < 0)
         {
            logit ("et", "%s(%s): Unknown datatype <%s>.\n",
                    MyProgName, MyModName, WaveBuf.trh2.datatype );
            continue;
         }

         /* Load decompressed data samples (which are in local */
         /* byte order thanks to brtt_genuncompress) into message;  */
         /* set new (real) length of the output message        */
         if( WaveBuf.trh2.datatype[1] == '2' )
         {
            for (i = 0; i < decompress_nout; i++) {
               WaveShort[i] = (short) decompress_out[i];
            }
            WaveBufLen = (decompress_nout * sizeof (short)) +
                          sizeof(TRACE2_HEADER);
         }
         else if( WaveBuf.trh2.datatype[1] == '4' )
         {
            for (i = 0; i < decompress_nout; i++) {
               WaveLong[i] = (int32_t) decompress_out[i];
            }
            WaveBufLen = (decompress_nout * sizeof (int32_t)) +
                         sizeof(TRACE2_HEADER);
         }
         else
         {
            logit("e","Unknown datatype <%s> in compressed waveheader\n",
                  WaveBuf.trh2.datatype );
            continue;
         }


      /* Write the message to shared memory
       ************************************/
         if( reclogo.type == TypeComp2 ) logo.type = TypeTraceBuf2;
         else                            logo.type = TypeTraceBuf;

         if( UseOriginalLogo ) 
         {
            logo.instid = reclogo.instid;
            logo.mod    = reclogo.mod;
         }
         if( tport_putmsg( &OutRegion, &logo, WaveBufLen, WaveBuf.msg ) != PUT_OK )
         {
            logit ("et",
                   "decompress_UA:  Error sending message: i%d m%d t%d.\n",
                    logo.instid, logo.mod, logo.type);
         }

      } while(res != GET_NONE);  /*end of message-processing-loop */

      sleep_ew (10);  /* no more messages; wait for new ones to arrive */

   } /* while (1) */

ShutDown:

/* Free malloc'd memory
 **********************/
   free (GetLogo);
   free (decompress_out);

/* Detach from shared memory
 ***************************/
   tport_detach (&InRegion);
   tport_detach (&OutRegion);
   fflush (stdout);
   return (exitstatus);
}

/******************************************************************************
 *  decompress_config() processes command file(s) using kom.c functions;      *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int decompress_config (char *configfile)
{
   char   init[NUM_COMMANDS];/* init flags, one byte for each reqd command */
   int    nmiss;             /* number of reqd commands that were missed   */
   char  *com;
   char  *str;
   int    nfiles;
   int    success;
   int    i;

	/* Set to zero one init flag for each required command
	*****************************************************/
	for (i = 0; i < NUM_COMMANDS; i++) init[i] = 0;

	/* Open the main configuration file
	**********************************/
	nfiles = k_open (configfile);
	if (nfiles == 0)
	{
		logit ("e",
			"decompress_UA: Error opening command file <%s>; exiting!\n",
			configfile);
		return EW_FAILURE;
	}

	/* Process all command files
	***************************/
	while(nfiles > 0)   /* While there are command files open */
	{
		while(k_rd ())        /* Read next line from active file  */
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
				  	  "decompress_UA: Error opening command file <%s>; exiting!\n",
					  &com[1]);
					return EW_FAILURE;
				}
				continue;
			}

			/* Process anything else as a command
			************************************/
			if (k_its ("MyModId"))
			{
				str = k_str ();
				if (str) strcpy
					(MyModName, str);
				init[0] = 1;
			}
			else if (k_its ("InRing"))
			{
				str = k_str ();
				if (str)
					strcpy (InRingName, str);
				init[1] = 1;
			}
			else if (k_its ("OutRing"))
			{
				str = k_str ();
				if (str)
					strcpy (OutRingName, str);
				init[2] = 1;
			}
			else if (k_its ("HeartBeatInterval"))
			{
				HeartBeatInterval = k_long ();
				init[3] = 1;
			}

			else if ( k_its( "GetLogo" ) )  
			{
				MSG_LOGO *tlogo = NULL;
				tlogo = (MSG_LOGO *)realloc( GetLogo, (nLogo+1)*sizeof(MSG_LOGO) );
				if( tlogo == NULL )
				{
				   logit( "e", 
                                          "decompress_UA: GetLogo: error reallocing"
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
				             "decompress_UA: GetLogo: invalid installation `%s'"
				             " in `%s'; exiting!\n", str, configfile );
				      return EW_FAILURE;
				   } 
				}   
				if( ( str = k_str() ) != NULL )
				{
				   if ( GetModId( str, &(GetLogo[nLogo].mod) ) != 0 )
				   { 
				      logit( "e",
				             "decompress_UA: GetLogo: invalid module id `%s'"
				             " in `%s'; exiting!\n", str, configfile );
				      return EW_FAILURE;
				   } 
				}   
				if( ( str = k_str() ) != NULL )
				{
				   if ( strcmp( str, "TYPE_TRACE2_COMP_UA" )!=0 &&
				        strcmp( str, "TYPE_TRACE_COMP_UA"  )!=0    )
				   { 
				      logit( "e",
				             "decompress_UA: GetLogo: invalid msgtype `%s'"
				             " in `%s'; must be either TYPE_TRACE_COMP_UA or"
				             " TYPE_TRACE2_COMP_UA; exiting!\n", str, configfile );
				      return EW_FAILURE;
				   } 
    				   if ( GetType( str, &(GetLogo[nLogo].type) ) != 0 )
				   { 
				      logit( "e",
				             "decompress_UA: GetLogo: invalid msgtype `%s'"
				             " in `%s'; exiting!\n", str, configfile );
				      return EW_FAILURE;
				   } 
				}   
				nLogo++;
				init[4] = 1;
			} /* end GetLogo */

                        else if( k_its("LogFile") )
                        {
                                LogSwitch = k_int();
                                init[5] = 1;
                        }

      /*optional*/      else if( k_its("UseOriginalLogo") )
                        {     
                                UseOriginalLogo = k_int();
                        }

			/* Unknown command
			*****************/
			else
			{
				logit ("e", "decompress_UA: <%s> Unknown command in <%s>.\n",
								com, configfile);
				continue;
			}

			/* See if there were any errors processing the command
			*****************************************************/
			if (k_err ())
			{
				logit ("e",
					"decompress_UA: Bad <%s> command in <%s>; exiting!\n",
				        com, configfile);
				return EW_FAILURE;
			}

		} /** while k_rd() **/

		nfiles = k_close ();

	} /** while nfiles **/

	/* After all files are closed, check init flags for missed commands
	******************************************************************/
	nmiss = 0;
	for (i = 0; i < NUM_COMMANDS; i++) if(!init[i]) nmiss++;

	if (nmiss)
	{
		logit ("e", "decompress_UA: ERROR, no " );
		if (!init[0])  logit ("e", "<MyModId> "           );
		if (!init[1])  logit ("e", "<InRing> "            );
		if (!init[2])  logit ("e", "<OutRing> "           );
		if (!init[3])  logit ("e", "<HeartBeatInterval> " );
		if (!init[4])  logit ("e", "<GetLogo> "           );
		if (!init[5])  logit ("e", "<LogFile> "           );
		logit ("e", "command(s) in <%s>; exiting!\n", configfile);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/******************************************************************************
 *  decompress_lookup( )   Look up important info from earthworm.h tables     *
 ******************************************************************************/
static int decompress_lookup (void)
{

	/* Look up keys to shared memory regions
	*************************************/
	if ((InKey = GetKey (InRingName)) == -1)
	{
		logit( "e",      
			"decompress_UA:  Invalid ring name <%s>; exiting!\n", InRingName);
		return EW_FAILURE;
	}

	if ((OutKey = GetKey (OutRingName)) == -1)
	{
		logit( "e",      
			"decompress_UA:  Invalid ring name <%s>; exiting!\n", OutRingName);
		return EW_FAILURE;
	}

	/* Look up installations of interest
	*********************************/
	if (GetLocalInst (&InstId) != 0)
	{
		logit( "e",      
		  "decompress_UA: error getting local installation id; exiting!\n");
		return EW_FAILURE;
	}

	/* Look up modules of interest
	***************************/
	if (GetModId (MyModName, &MyModId) != 0)
	{
		logit( "e",      
		  "decompress_UA: Invalid module name <%s>; exiting!\n", MyModName);
		return EW_FAILURE;
	}

	/* Look up message types of interest
	*********************************/
	if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0)
	{
		logit( "e",      
		  "decompress_UA: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_ERROR", &TypeError) != 0)
	{
		logit( "e",      
		  "decompress_UA: Invalid message type <TYPE_ERROR>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_TRACEBUF", &TypeTraceBuf) != 0)
	{
		logit( "e",      
		  "decompress_UA: Invalid message type <TYPE_TRACEBUF>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_TRACEBUF2", &TypeTraceBuf2) != 0)
	{
		logit( "e",      
		  "decompress_UA: Invalid message type <TYPE_TRACEBUF2>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_TRACE_COMP_UA", &TypeComp) != 0)
	{
		logit( "e",      
		  "decompress_UA: Invalid message type <TYPE_TRACE_COMP_UA>; exiting!\n");
		return EW_FAILURE;
	}
	if (GetType ("TYPE_TRACE2_COMP_UA", &TypeComp2) != 0)
	{
		logit( "e",      
		  "decompress_UA: Invalid message type <TYPE_TRACE2_COMP_UA>; exiting!\n");
		return EW_FAILURE;
	}

	return EW_SUCCESS;

}

/******************************************************************************
 * decompress_status() builds a heartbeat or error message & puts it into     *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void decompress_status( unsigned char type, short ierr, char *note )
{

	MSG_LOGO    logo;
	char        msg[256];
	long        size;
	time_t      t;

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
		logit ("et", "decompress_UA: %s\n", note);
	}

	size = (long)strlen (msg);   /* don't include the null byte in the message */

	/* Write the message to shared memory
	************************************/
	if (tport_putmsg (&OutRegion, &logo, size, msg) != PUT_OK)
	{
		if (type == TypeHeartBeat)
		{
		   logit ("et", "decompress_UA:  Error sending heartbeat.\n");
		}
		else if (type == TypeError)
		{
		   logit ("et", "decompress_UA:  Error sending error:%d.\n", ierr);
		}
	}

}


/******************************************************************************
 * brtt_genuncompress() - generic decompression routine; takes an input byte  *
 *                   array and decompresses it into an output integer array   *
 *                                                                            *
 *    Copyright (c) 1997 Boulder Real Time Technologies, Inc.                 *
 *                                                                            *
 ******************************************************************************/
static int brtt_genuncompress (int **out, int *nout, int *size,
                               unsigned char *in, int nbytes)
{
	int n=0;
	int i=0, j=0, k=0;
	int msbit=0;
	int length=0;
	int gc_nsamp=0;
	int gc_length=0;
	int nin=0;
	int *bout;

	i=0; j=0; nin=0;
	if (*out == NULL)
	{
		*size = 5*nbytes;
		*out = (int *) malloc (*size);
		if (*out == NULL)
		{
			logit( "e", "brtt_genuncompress: malloc() error.\n");
			return EW_FAILURE;
		}
	}

	bout = *out;
	gc_nsamp = ntohl(*((int *)in));
	nin = 4;
	gc_length = in[nin];
	nin++;
	*nout = 0;
	while (nin < nbytes)
	{
		length = in[nin] + 1;
		nin++;
		msbit=7;
		for (k=0; k<gc_length; k++)
		{
			*bout = 0;
 			bitunpack (bout, &nin, &msbit, length, in);
			*bout = ((*bout)<<(32-length))>>(32-length);

			bout++;
			n++;
			while (n * sizeof (int) >= (unsigned) *size)
			{
				int offset; /*added to make NT happy. LDD*/
			     /* *size *= (int) 1.2; */      /*BUG: same as *size *= 1; sets up infinite loop. LDD */
                                *size = (int)(1.2 * *size); /*FIX: multiply in float, then change to int. LDD */
	   		     /* bout -= (int)(*out); */ /*NT didn't like this. LDD*/
				offset = (int)(bout - *out);   /*changed to make NT happy. LDD*/
				*out = (int *) realloc (*out, *size);
				if (*out == NULL)
				{
					logit( "e", "brtt_genuncompress: realloc() error.\n");
					return EW_FAILURE;
				}
			     /* bout += (int)(*out); */ /*NT didn't like this. LDD*/
				bout = *out + offset;   /*changed to make NT happy. LDD*/
                        }
			if (n >= *nout + gc_nsamp) break;
		}
		if (msbit != 7)
			nin++;

		if (n >= *nout + gc_nsamp)
		{
			*nout += gc_nsamp;
			if (nin >= nbytes) break;
			in += nin;
			nbytes -= nin;
			memcpy (&gc_nsamp, in, 4);
			gc_nsamp = ntohl(gc_nsamp);
			nin = 4;
			gc_length = in[nin];
			nin++;
		}
	}

	return EW_SUCCESS;
}


/******************************************************************************
 * bitunpack () - part of the decompression routine                           *
 *                                                                            *
 *    Copyright (c) 1997 Boulder Real Time Technologies, Inc.                 *
 *                                                                            *
 ******************************************************************************/
static void bitunpack (int *out, int *n, int *msbit,
                      int length, unsigned char *in)
{
	int lsbit=0;
	int mask=0;
	int nn=0;
	int i=0;

	lsbit = *msbit - length + 1;
	if (lsbit < 0)
	{
		nn = *msbit + 1;
		mask = ((1<<nn)-1)<<(-lsbit);
		i = in[*n];
		*out |= ((i<<(-lsbit)) & mask);
		(*n)++;
		*msbit = 7;
		bitunpack (out, n, msbit, length-nn, in);
	}
	else
	{
		mask = (1<<length)-1;
		if (lsbit == 0)
		{
			*out |= (in[*n] & mask);
			(*n)++;
			*msbit = 7;
		}
		else
		{
			*out |= ((in[*n]>>lsbit) & mask);
			(*msbit) -= length;
		}
	}
}

