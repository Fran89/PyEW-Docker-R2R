
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ew2seisvole.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2007/02/26 16:48:07  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.5  2002/05/15 17:06:52  patton
 *     Made logit changes.
 *
 *     Revision 1.4  2001/05/09 17:29:19  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.3  2000/08/08 18:23:27  lucky
 *     Lint cleanup
 *
 *     Revision 1.2  2000/07/24 20:13:22  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 17:20:06  lucky
 *     Initial revision
 *
 *
 */

/*       *************************
 * ew2seisvole.c:


   This module was derived from cubic_msg. It interfaces to Alan
   Jones' Seisvole display program: it leaves files containing 
   cubic messages in the directory specified by the TargetDirectory
   option from the ew2seisvole.d file. The idea is that Seisvole is
   hopefully configured to scan this directory (every 10 seconds), 
   and read and delete any files it finds there.  Seisvole builds its 
   own internal cataloge of events from that.  

	Lucky Vidmar
    Mon Feb 15 10:27:09 MST 1999


Mon Dec 21 10:23:26 MST 1998 lucky:

      Log file name will be built from the name of the configuration
      file -- argv[1] passed to logit_init().

      Process id is sent with the heartbeat for restart purposes.

      Flush the input ring buffer before proceding to the main loop.

      Calls to DECODE() macro replaced by their full equivalents 
      because DECODE was not MT safe.

      ===================== Y2K compliance =====================
      Formats in makeCubicMsg() changed (among other
      things)to include date in the form YYYYMMDD.

      Message type names changed to their Y2K equivalents. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>

static  SHM_INFO  Region1;          /* public shared memory for receiving summary messages*/
static  SHM_INFO  Region2;          /* ew2seisvole's private memory         */
const long region2_size = 10000;    /* size of ew2seisvole's private memory */


/* Things to lookup in the earthworm.h table with getutil.c functions
 **********************************************************************/
static long          MyRingKey;         /* key to ew2seisvole's private ring region */
static long          PublicKey;         /* key to pbulic memory region for i/o     */
static unsigned char InstId;            /* local installation id      */
static unsigned char MyModId;           /* our module id             */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeSumm;
static unsigned char TypeSeismic;

/* Things to read from configuration file
 ****************************************/
static char MyModuleId[MAX_MOD_STR];       /* module id for this module              */
static char RingName[MAX_RING_STR];         /* name of transport ring for i/o         */
static int  LogSwitch;            /* 0 if no logging should be done to disk */

#define  MAXLOGO   1
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid  */
short     nLogo;                  /* number of logos read from ring           */

double    MagThreshold;           /* Mag threshold above which we send a page */
char      TargetDirectory[1024];  /* Mag threshold above which we send a page */


/* Variables for talking to errmgr
 *********************************/
time_t    timeNow;
time_t    timeLastBeat;              /* time last heartbeat was sent */
time_t    heartBeatInterval = 5;    /* seconds between heartbeats   */
char      Text[150];


pid_t   MyPid;  /** Out process id is sent with heartbeat **/ 


/* Error words used by ew2seisvole
 ********************************/
#define   ERR_MISSMSG           0
#define   ERR_TOOBIG            1
#define   ERR_NOTRACK           2
#define   ERR_INTERNAL          3


/* Functions in this source file
 ******************************/
void ew2seisvole_status( unsigned char, short, char *); /* sends heartbeats and errors into ring*/
void ew2seisvole_config( char * );          /* reads configuration (.d) file via Carl's routines */
void ew2seisvole_lookup ( void );           /* Goes from symbolic names to numeric values, via earthworm.h */
char* makeCubicMsg( char* );     /* formats a pretty message. args: hypo71sumCard, . */
int isItBigDeal(char*, double);        /* to see if it's worth notifying. args: hypo71sumCard, MagThreshold */
int sendMessage(char*, unsigned char); /* send message to HYPO Ring. args: message text, message type */
unsigned int cksum( char * );

/* **************************************************************************/

int main( int argc, char **argv )
{
        char        message[128];                 /* actual retrieved message   */
        long        msgSize;                     /* size of retrieved message  */
        MSG_LOGO    msgLogo;                     /* logo of retrieved message  */
        char        h71sum[128];                 /* hypo71 summary card */
        int         res;
        char*       msgtext;
        char*       flushbuf; /* tmp buffer used for flushing input ring */
        char        filename[1024];
        FILE        *fp;

        /* Check command line arguments
         ******************************/
        if ( argc != 2 )
        {
        fprintf( stderr, "Usage: ew2seisvole <configfile>\n" );
        return -1;
        }

		/* Initialize name of log-file & open it
        ***************************************/
        logit_init( argv[1], 0, 256, 1 );

        /* Read the configuration file(s)
        ********************************/
        ew2seisvole_config( argv[1] );
		logit( "" , "ew2seisvole: Read command file <%s>\n", argv[1] );

        /* Look up important info from earthworm.h tables
        ************************************************/
        ew2seisvole_lookup();

        /* Reinitialize logit to the desired logging level
        **************************************************/
        logit_init( argv[1], 0, 256, LogSwitch );
        

    /* write some of our parameters to the log file
     **********************************************/
        logit("","ew2seisvole: MyModuleId: %s \n",MyModuleId);
        logit("","ew2seisvole: RingName: %s \n",RingName);
        logit("","ew2seisvole: MagThreshold: %f \n",MagThreshold);
        logit("","ew2seisvole: GetLogo: %u %u %u\n",GetLogo[0].instid, GetLogo[0].mod, GetLogo[0].type);
        logit("","ew2seisvole: CUBIC nLogo: %d \n",nLogo);


    /* Get our own process ID for restart purposes
     **********************************************/
        if ((MyPid = getpid ()) == -1)
		{
            logit ("e", "cobuc_msg: Call to getpid failed. Exiting.\n");
            exit (-1);
		}
    
    
        /* Attach to public HYPO shared memory ring
        ******************************************/
        tport_attach( &Region1, PublicKey );
        logit( "", "ew2seisvole: Attached to public memory region <%s>: %ld.\n", RingName, Region1.key );


        /* Flush the input ring */
        if ((flushbuf = (char *) malloc (MAX_BYTES_PER_EQ)) == NULL)
        {
            logit ("e", "ew2seisvole: error allocating flushbuf; exiting!\n");
            exit (-1);
        }

        /* Create ew2seisvole's private memory ring for buffering input
        *********************************************************/
        tport_create( &Region2, region2_size, MyRingKey );
        logit( "", "ew2seisvole: Attached to private memory region: %ld.\n", Region2.key );

      
        while (tport_getmsg( &Region2, GetLogo, nLogo, &msgLogo, 
                   &msgSize, message, sizeof(message)-1 ) != GET_NONE)
            ;

        free (flushbuf);


        /* Send first heartbeat
        **********************/
        time(&timeLastBeat);
        ew2seisvole_status( TypeHeartBeat, 0, "" );

        /* Start input buffer thread
        ***************************/
        if ( tport_buffer( &Region1, &Region2, GetLogo, nLogo, sizeof(message)-1, MyModId, InstId ) == -1 )
                {
                tport_destroy( &Region2 );
                logit( "et", "ew2seisvole: Error starting input buffer thread; exiting!\n" );
                return -1;
                }
        logit( "t", "ew2seisvole: Started input buffer thread.\n" );


        /* ------------------------ start working loop -------------------------*/
        while(1)
                {
                do
                        {
                        /* see if a termination has been requested */
                        /* *************************************** */
                        if ( tport_getflag( &Region1 ) == TERMINATE ||
                             tport_getflag( &Region1 ) == MyPid )
                                {
                                /* detach from shared memory regions*/
                                tport_detach( &Region1 );
                                tport_destroy( &Region2 );
                                logit("t", "ew2seisvole: Termination requested; exiting.\n" );
                                return 0;
                                }

                        /* send ew2seisvole's heartbeat
                        *****************************/
                        if  ( time(&timeNow) - timeLastBeat  >=  heartBeatInterval )
                                {
                                timeLastBeat = timeNow;
                                ew2seisvole_status( TypeHeartBeat, 0, "" );
                                }

                        /* Get and process the next hyposum message from shared memory */
                        /***************************************************************/
                        res = tport_getmsg( &Region2, GetLogo, nLogo, &msgLogo, &msgSize,
                                             message, sizeof(message)-1 );
                        switch(res)
                                {
                                case GET_NONE:
                                break;

                                case GET_TOOBIG:
                                sprintf( Text, "Retrieved msg[%ld] (i%u m%u t%u) too big for message[%d]",
                                         msgSize,msgLogo.instid, msgLogo.mod, msgLogo.type, (int)sizeof(message) );
                                ew2seisvole_status( TypeError, ERR_TOOBIG, Text );
                                break;

                                case GET_MISS:
                                sprintf( Text,"Missed msg(s)  i%u m%u t%u  region:%ld.",
                                        msgLogo.instid, msgLogo.mod, msgLogo.type, Region2.key);
                                ew2seisvole_status( TypeError, ERR_MISSMSG, Text );

                                case GET_NOTRACK:
                                sprintf( Text,"Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                                        msgLogo.instid, msgLogo.mod, msgLogo.type );
                                ew2seisvole_status( TypeError, ERR_NOTRACK, Text );

                                case GET_OK:
                                message[msgSize] = '\0';              /*null terminate the message*/
                                /* copy message to hy71sum      */
                                strcpy( h71sum,message);

                                /* See if its a big deal, and create a message to pagerfeeder if so
                                *******************************************************************/
                                if  ( isItBigDeal(h71sum, MagThreshold)==1)
                                {
                                        msgtext = makeCubicMsg (h71sum);
                    logit("et","ew2seisvole generated message:\n <<%s>> \n", msgtext);

                                    if (msgtext==NULL)
                                    {
                                        sprintf(Text,"ew2seisvole: makeCubicMsg failed, args: %s ",h71sum);
                                        ew2seisvole_status(TypeError,ERR_INTERNAL,Text);
                                    }

                                     /* Write out the message */

                                    /* Build a unique file name */
                                    sprintf (filename, "%sew2seisvole-%ld", TargetDirectory, (long)time(NULL));

                                    /* Open the file */
                                    if ((fp = fopen (filename, "wt")) == NULL) 
                                    {
                                        sprintf(Text,"ew2seisvole: Can't open file %s\n", filename);
                                        ew2seisvole_status(TypeError,ERR_INTERNAL,Text);
                                    }
                                      

                                    /* Write the message to the file */
                                    if (fprintf (fp, "%s", msgtext) < 0)
                                    {
                                        sprintf(Text,"ew2seisvole: Can't write to file %s\n", filename);
                                        ew2seisvole_status(TypeError,ERR_INTERNAL,Text);
                                    }

                                    /* Close the file */
                                    fclose (fp);

                                }
                                break;
                                }
                        } while (res !=GET_NONE );      /* end of message processing loop */

                        sleep_ew( 500 );       /* wait around for more summary lines   */
                }

   /*------------------------------end of working loop------------------------------*/
}

/****************************************************************************
 *      ew2seisvole_config() process command file using kom.c functions
 *                      exits if any errors are encounterd
 ****************************************************************************/

void ew2seisvole_config( char* configfile )
{
   int  ncommand;       /* # of required commands you expect */
   char init[10];       /* init flags, one byte for each required command */
   int  nmiss;          /* number of required commands that were missed   */
   char *com;
   char *str;
   int  nfiles;
   int  success;
   int  i;


/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 6;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo    = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit("e",
                "ew2seisvole: Error opening command file <%s>; exiting!\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  logit("e",
                          "ew2seisvole: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("LogSwitch") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModuleId , str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }

        /* Retrieve installation and module to get messages from  */

  /*3*/     else if( k_its("GetSumFrom") ) {
                if ( nLogo >= MAXLOGO ) {
                    logit("e",
                            "ew2seisvole: Too many <GetSumFrom> commands in <%s>",
                             configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit("e",
                               "ew2seisvole: Invalid installation name <%s>", str );
                       logit("e", " in <GetSumFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit("e",
                               "ew2seisvole: Invalid module name <%s>", str );
                       logit("e", " in <GetSumFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                if( GetType( "TYPE_H71SUM2K", &GetLogo[nLogo].type ) != 0 ) {
                    logit("e",
                               "ew2seisvole: Invalid message type <TYPE_H71SUM2K>" );
                    logit("e", "; exiting!\n" );
                    exit( -1 );
                }
        /*
                printf("ew2seisvole: GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type );
        */
                nLogo++;
                init[3] = 1;
            }

        /* Get the magnitude threshold
         ****************************/
/*4*/   else if( k_its("MagThreshold") ) {
                MagThreshold=k_val();
                init[4] = 1;
        }

        /* Get the target directory
         ****************************/
/*5*/   else if( k_its("TargetDirectory") ) 
        {
                str = k_str();
                if(str) strcpy( TargetDirectory, str );
                init[5] = 1;
        }

        /* At this point we give up. Unknowd thing.
        *******************************************/
        else {
                logit("e", "ew2seisvole: <%s> Unknown command in <%s>.\n",
                        com, configfile );
                continue;
        }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit("e", "ew2seisvole: Bad <%s> command  in <%s>; exiting!\n",
                        com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit("e", "ew2seisvole: ERROR, no " );
       if ( !init[0] )  logit("e", "<LogSwitch> "      );
       if ( !init[1] )  logit("e", "<MyModuleId> "   );
       if ( !init[2] )  logit("e", "<RingName> "     );
       if ( !init[3] )  logit("e", "<GetSumFrom> " );
       if ( !init[4] )  logit("e", "<MagThreshold> " );
       if ( !init[5] )  logit("e", "<TargetDirectory> " );
       logit("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

  return;
}


/******************************************************************************/
/* ew2seisvole_status() builds a heartbeat or error msg & puts it into shared memory */
/******************************************************************************/
void ew2seisvole_status( unsigned char type,  short ierr,  char *note )
{
        MSG_LOGO    logo;
        char        msg[256];
        long        size;
        time_t      t;

        logo.instid = InstId;
        logo.mod    = MyModId;
        logo.type   = type;

        time( &t );
        if( type == TypeHeartBeat ) {
                sprintf( msg, "%ld %d\n", (long) t, MyPid ); 
        }
        else if( type == TypeError ) {
                sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note );
                logit( "t", "ew2seisvole:  %s\n", note );
        }

        size = (long)strlen( msg );   /* don't include the null byte in the message */
        if( tport_putmsg( &Region1, &logo, size, msg ) != PUT_OK )
        {
                if( type == TypeHeartBeat ) {
                    logit("et","ew2seisvole:  Error sending heartbeat.\n" );
                }
                else if( type == TypeError ) {
                    logit("et","ew2seisvole:  Error sending error:%d.\n", ierr );
                }
        }
        return;
}


/*********************************************************************************/
/*  ew2seisvole_lookup( ) Look up important info from earthworm.h tables                 */
/*********************************************************************************/
void ew2seisvole_lookup( )
{
/* Look up keys to shared memory regions
   *************************************/
   if( (PublicKey = GetKey(RingName)) == -1 ) {
        fprintf( stderr,
                "ew2seisvole: Invalid ring name <%s>; exiting!\n", RingName );
        exit( -1 );
   }
   if( (MyRingKey = GetKey("CUBIC_RING")) == -1 ) {
        fprintf( stderr,
                "ew2seisvole: Invalid ring name <CUBIC_RING>; exiting!\n" );
        exit( -1 );
   }

/* Look up installation Id
   ***********************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "ew2seisvole: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModuleId, &MyModId ) != 0 ) {
      fprintf( stderr,
              "ew2seisvole: Invalid module name <%s>; exiting!\n", MyModuleId );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "ew2seisvole: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "ew2seisvole: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_H71SUM2K", &TypeSumm) != 0 ) {
      fprintf( stderr,
              "ew2seisvole: Invalid message type <TYPE_H71SUM2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_CUBIC", &TypeSeismic ) != 0 ) {
      fprintf( stderr,
              "ew2seisvole: Invalid message type <TYPE_PAGE>; exiting!\n" );
      exit( -1 );
   }
     return;
}
/***********************************************************************/
/* makeCubicMsg: make up a cubic message                   */
/***********************************************************************/
/*
H71SUM2K SUMMARY FORMAT. As salvaged from BJ's stuff

 ================= Y2k =================
 Besides adding two digits to the year the format of
 the type has changed. The defines below indicate
 the starting column of each filed (B_FIELDNAME)
 and the length of the field (L_FIELDNAME).


Cols.  len       Data
_____  ______    ____

0-7    8         Year, month and day. yyyymmdd
9-12   4         Hour and minute.
13-18  6         Origin time seconds.
19-21  3         Latitude (deg).
22     1         S for south, blank otherwise.
23-27  5         Latitude (min).
28-31  4         Longitude (deg).
32     1         E for east, blank otherwise.
33-37  5         Longitude (min).
38-44  7         Depth (km).
46     1         Preferred mag. label
47-51  5         Preferred magnitude.
52-54  3         Number of P & S times with weights greater than 0.1.
55-58  4         Maximum azimuthal gap.
59-63  5         Distance to nearest station (km).
64-68  5         RMS travel time residual.
69-73  5         Horizontal error (km).
74-78  5         Vertical error (km).
80     1         Quality (A-D)
81     1         Machine code (A for Earthworm)

==== Y2K compliant output message ====

19960508 2005 44.83 38 47.53 122 45.28   2.56 D 0.86 30  43  4.  0.07 0.2  0.5 AW   51056678  \n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 12345

Output message

  CUBE "E " format for cubic events from Peter Germain
           fixed format by column:

      TpEidnumbrSoVYearMoDyHrMnSecLatddddLongddddDept...
      12345678901234567890123456789012345678901234567
               1         2         3         4

      continued:...MgNstNphDminRmssErhoErzzGpMNmEmLC
                   890123456789012345678901234567890
                     5         6         7         8

>a2 * Tp   = Message type = "E " (cubic event)
>a8 * Eid  = Event identification number  (any string)
>a2 * So   = Data Source =  regional network designation
>a1 * V    = Event Version     (ASCII char, except [,])
>i4 * Year = Calendar year                (GMT) (-999-6070)
>i2 * Mo   = Month of the year            (GMT) (1-12)
>i2 * Dy   = Day of the month             (GMT) (1-31)
>i2 * Hr   = Hours since midnight         (GMT) (0-23)
>i2 * Mn   = Minutes past the hour        (GMT) (0-59)
>i3 * Sec  = Seconds past the minute * 10 (GMT) (0-599)
>i7 * Lat  = Latitude:  signed decimal degrees*10000 north>0
>i8 * Long = Longitude: signed decimal degrees*10000 west <0
>I4   Dept = Depth below sea level, kilometers * 10
>I2   Mg   = Magnitude * 10
>I3   Nst  = Number of stations used for location
>I3   Nph  = Number of phases used for location
>I4   Dmin = Distance to 1st station;   kilometers * 10
>I4   Rmss = Rms time error; sec * 100
>I4   Erho = Horizontal standard error; kilometers * 10
>I4   Erzz = Vertical standard error;   kilometers * 10
>I2   Gp   = Azimuthal gap, percent of circle; degrees/3.6
>a1   M    = Magnitude type
>I2   Nm   = Number of stations for magnitude determination
>I2   Em   = Standard error of the magnitude * 10
>a1   L    = Location method
>a1 * C    = Menlo Park check character, defined below


*/


#define B_DATE      0       /* Year, month and day - yyyymmdd */
#define L_DATE          8
#define B_HOUR      9       /* Hour */
#define L_HOUR          2
#define B_MIN       11      /* Minute */
#define L_MIN           2
#define B_SEC1      14      /* Seconds first part */
#define L_SEC1          2
#define B_SEC2      17      /* Seconds second part */
#define L_SEC2          1
#define B_LAT_D     19      /* Latitude - degrees */
#define L_LAT_D         3
#define B_LAT_M     23      /* Latitude - minutes */
#define L_LAT_M         5
#define LAT_IND         22  /* South or North */
#define B_LON_D     28      /* Longitude - degrees */
#define L_LON_D         4
#define B_LON_M     33      /* Longitude - minutes */
#define L_LON_M         5
#define LON_IND         32  /* East or West */
#define B_DEPTH1    39      /* Depth first component */
#define L_DEPTH1        3
#define B_DEPTH2    43      /* Depth second component */
#define L_DEPTH2        1
#define MAG_LABEL_IND   46  /* Preferred magnitude label */
#define B_MAG1      48      /* Magnitude first component */
#define L_MAG1          1
#define B_MAG2      50      /* Magnitude second component */
#define L_MAG2          1
#define B_PS        52      /* Number of P&S stations used */
#define L_PS            3
#define B_GAP       56      /* Maximum asimuthal gap */
#define L_GAP           3
#define B_DMIN      59      /* Distance to nearest stations */
#define L_DMIN          3
#define B_RMS1      64      /* RMS travel time residual */
#define L_RMS1          2
#define B_RMS2      67      /* RMS travel time residual */
#define L_RMS2          2
#define B_ERH1      69      /* Horizontal error - first component */
#define L_ERH1          3
#define B_ERH2      73      /* Horizontal error - second component */
#define L_ERH2          1
#define B_ERV1      74      /* Vertical error - first component */
#define L_ERV1          3
#define B_ERV2      78      /* Vertical error - second component */
#define L_ERV2          1
#define DSC_IND         81  /* Data Source code */
#define B_EID       85      /* last 8 digits of the event ID number */
#define L_EID           8

char* makeCubicMsg(char* sumCard)
{
#define CUBIC_MSG_LEN 80
        static char cubicMsg[CUBIC_MSG_LEN];   /* Common max alpha pager message length */
        int i;
    char    junk[50];
    char    tmpbuf[50];

        /* make up a cubic message - from hypo71 summary format
        *******************************************************/
    /* Code from broadcast.c by Andy Michael */
    /*****************************************/
        for (i=0;i<CUBIC_MSG_LEN;i++) cubicMsg[i]=' '; /*fill with spaces */

    /* Type A Earthquake message */
    strcpy(cubicMsg, "E ");

    /* Event ID number  */
        strncat(cubicMsg, &sumCard[B_EID], L_EID);

        /* Data source code */
    if (sumCard[DSC_IND]=='U')
        /* First transmission and possibly US Net */
        strcat(cubicMsg, "US");
    else
        /* First transmission and Northern California Net */
        strcat(cubicMsg, "NC");

    /* Set the update flag to initial on earthworm */
        strcat(cubicMsg, "0");

    /* Date yyyymmdd*/
    strncat(cubicMsg, &sumCard[B_DATE], L_DATE);
    /* Time hh */
    strncat(cubicMsg, &sumCard[B_HOUR], L_HOUR);
    /* Time mm */
    strncat(cubicMsg, &sumCard[B_MIN], L_MIN);
    /* Time sss */
    strncat(cubicMsg, &sumCard[B_SEC1], L_SEC1);
    strncat(cubicMsg, &sumCard[B_SEC2], L_SEC2);

    /* Latitude */
    /* Integer part */
    if (sumCard[B_LAT_D]==' ') {
        if (sumCard[LAT_IND]=='S' || sumCard[LAT_IND]=='s')
            /* Southern hemisphere */
            sumCard[B_LAT_D] = '-';
        else
            /* Default: Northern hemisphere */
            sumCard[B_LAT_D]='+';
    }
        for (i = B_LAT_D; i < (B_LAT_D + L_LAT_D); ++i)
        if (sumCard[i]==' ')
            sumCard[i]='0';
    strncat(cubicMsg, &sumCard[B_LAT_D], L_LAT_D);

    /* Fractional part */
    if (strncpy (tmpbuf, &sumCard[B_LAT_M], L_LAT_M) == NULL)
    {
        fprintf (stderr, "makeCubicMsg: call to strncpy failed.\n");
        return NULL;
    }
    tmpbuf[L_LAT_M] = '\0';

    sprintf(junk, "%6.4f", ((atof (tmpbuf))/60.0) );
    strncat(cubicMsg, &junk[2], 4);

    /* Longitude */
    /* Integer part */
    if (sumCard[B_LON_D]==' ') {
        if (sumCard[LON_IND]=='E' || sumCard[LON_IND]=='e')
            /* Eastern hemisphere */
            sumCard[B_LON_D] = '+';
        else
            /* Default: Western hemisphere */
            sumCard[B_LON_D]='-';
    }
        for (i=B_LON_D; i < (B_LON_D + L_LON_D); ++i)
        if (cubicMsg[i]==' ')
            cubicMsg[i]='0';
    strncat(cubicMsg, &sumCard[B_LON_D], L_LON_D);

    /* Fractional part */
    if (strncpy (tmpbuf, &sumCard[B_LON_M], L_LON_M) == NULL)
    {
        fprintf (stderr, "makeCubicMsg: call to strncpy failed.\n");
        return NULL;
    }
    tmpbuf[L_LON_M] = '\0';

    sprintf(junk, "%6.4f", ((atof (tmpbuf))/60.0) );
    strncat(cubicMsg, &junk[2], 4);


    /* Depth */
    strncat(cubicMsg, &sumCard[B_DEPTH1], L_DEPTH1);
    strncat(cubicMsg, &sumCard[B_DEPTH2], L_DEPTH2);

    /* Magnitude */
    strncat(cubicMsg, &sumCard[B_MAG1], L_MAG1);
    strncat(cubicMsg, &sumCard[B_MAG2], L_MAG2);

    /* Number of stations used */
    strncat(cubicMsg, &sumCard[B_PS], L_PS);

    /* Number of phases used */
    strncat(cubicMsg, &sumCard[B_PS], L_PS);

    /* DMin */
    strcat(cubicMsg, " "); /* Our cards can't have over 999 DMin */
    strncat(cubicMsg, &sumCard[B_DMIN], L_DMIN);

    /* RMS */
    strncat(cubicMsg, &sumCard[B_RMS1], L_RMS1);
    strncat(cubicMsg, &sumCard[B_RMS2], L_RMS2);

    /* Erh */
    strncat(cubicMsg, &sumCard[B_ERH1], L_ERH1);
    strncat(cubicMsg, &sumCard[B_ERH2], L_ERH2);

    /* Erz */
    strncat(cubicMsg, &sumCard[B_ERV1], L_ERV1);
    strncat(cubicMsg, &sumCard[B_ERV2], L_ERV2);

    /* Gap in % of a circle */
    if (strncpy (tmpbuf, &sumCard[B_GAP], L_GAP) == NULL)
    {
        fprintf (stderr, "makeCubicMsg: call to strncpy failed.\n");
        return NULL;
    }
    tmpbuf[L_GAP] = '\0';

    sprintf(junk, "%2f", ((atof (tmpbuf))/3.6) );
    strncat(cubicMsg, junk, 2);

    /* Mag type */
    junk[0]= (char)toupper(sumCard[MAG_LABEL_IND]);
    junk[1]='\0';
    strncat(cubicMsg, junk, 1);

    /* hypo71 card does not have NMag */
    strcat(cubicMsg, "  ");

    /* hypo71 card does not have EMag */
    strcat(cubicMsg, "  ");

    /* Type of timing */
    if (sumCard[DSC_IND]=='R' || sumCard[DSC_IND]=='M' || 
                                        sumCard[DSC_IND]=='P')
        strcat(cubicMsg, "R");
    else
        strcat(cubicMsg, "H");
    
    /* Checksum, expressed as single character */
    junk[0]= 36 + cksum(cubicMsg)%91;
    junk[1]='\0';
    strcat(cubicMsg, junk);

    /* Terminator */
    strcat(cubicMsg, "#\n");
        return(cubicMsg);
}

/****************************************************************************************************************
        isItBigDeal(hypo71sumCard,MagThresh)  to decide whether to send alarm or not
****************************************************************************************************************/
#define B_MAG   47  /* index into sumCard where magnitude begins */
#define L_MAG        5  /* length of field */

int isItBigDeal(char* sumCard, double magThreshold)
{
        static char magStr[L_MAG + 1];
        double magVal;

        /* extract mag from sum card, and do one if statement - big deal */
        strncpy(magStr,&sumCard[B_MAG],(size_t)L_MAG); /* magnitude */
        magStr[L_MAG] = '\0';
        magVal=atof(magStr);

        /* And here, the sophisticated decision is made...*/
        if (magVal>=magThreshold) return(1);
        return(0);
}


/********************************/
/* sendMessage(msg, msgtype)   */
/********************************/
int sendMessage( char* msg,  unsigned char type )
{
        MSG_LOGO    logo;
        long        size;

        logo.instid = InstId;
        logo.mod    = MyModId;
        logo.type   = type;

        size = (long)strlen( msg );   /* don't include the null byte in the message */
        if( tport_putmsg( &Region1, &logo, size, msg ) != PUT_OK )
                {
                logit("et","ew2seisvole:  Error sending cubic message.\n" );
                return(-1);
                }
        return(1);
}
