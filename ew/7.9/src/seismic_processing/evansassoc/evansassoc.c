
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: evansassoc.c 2892 2007-03-28 18:31:06Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2007/03/28 18:30:22  paulf
 *     removed malloc.h since it is now in platform.h and not used on some platforms
 *
 *     Revision 1.6  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.5  2004/05/24 20:37:38  dietz
 *     Added location code; uses TYPE_LPTRIG_SCNL as input,
 *     outputs TYPE_TRIGLIST_SCNL.
 *
 *     Revision 1.4  2002/06/05 15:59:11  patton
 *     Made logit changes.
 *
 *     Revision 1.3  2001/05/09 20:05:14  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or mypid.
 *
 *     Revision 1.2  2000/07/09 17:53:34  lombard
 *     Deleted unused variable TypeError
 *
 *     Revision 1.1  2000/02/14 17:15:33  lucky
 *     Initial revision
 *
 *
 */

           /****************************************************
            *                File evansassoc.c                 *
            *                                                  *
            *   Associator for long-period triggers detected   *
            *   by the John Evans algorithm (evanstrig)        *
            ****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <kom.h>
#include <trace_buf.h>
#include <transport.h>

#define TRG_SIZE 100          /* Max size of trigger messages */
#define NAME_LEN 50         

            /*********************************************
             *          Public Global Variables          *
             *********************************************/
unsigned char MyInstId;                /* Local installation */
unsigned char MyModuleId;              /* Our module id */
unsigned char TypeTrigListSCNL;        /* message type we'll create */
int           TriggerTimeLimit;        /* Purge triggers after this time */
int           CriticalNu;              /* Min station count for event detection */
int           CriticalMu;              /* Min station count for event continuation */
int           PreEventTime;            /* pre-event portion (s) of event record */
int           MinEventTime;            /* record length (s) of "normal" event */
int           MaxEventTime;            /* record length (s) of "big" event */
long          NextEventID;             /* next Eventid to use */
char          EventIDfile[NAME_LEN+1]; /* name of file containing NextEventID */

            /*********************************************
             *           Static Global Variables         *
             *********************************************/
static SHM_INFO      InRegion;         /* Info structure for memory InRegion */
static SHM_INFO      OutRegion;        /* Info structure for memory OutRegion */
static unsigned char GetThisModuleId;  /* Get messages only from this guy */
static long          InKey;            /* Key of InRegion to get trgs from */
static long          OutKey;           /* Key of OutRegion to write output to */
static int           LogSwitch=1;      /* if 0, don't write logfile */
static unsigned char GetThisInst;      /* Get msgs from this inst id */
static int           HeartbeatInt;     /* Heartbeat interval in seconds */
static unsigned char TypeLpTrigSCNL;   /* Get this message type */
static unsigned char TypeHeartBeat;

/* Structure for extra channels
 ******************************/
typedef struct _SCNL_STRUCT {
   char         sta[TRACE2_STA_LEN];
   char         chan[TRACE2_CHAN_LEN];
   char         net[TRACE2_NET_LEN];
   char         loc[TRACE2_LOC_LEN];
} SCNL_STRUCT;

static SCNL_STRUCT *ExtraSCNL;
static int nExtraSCNL   = 0;
static int MaxExtraSCNL = 0;


            /*********************************************
             *            Function declarations          *
             *********************************************/
void get_config( char * );
void lookup_ewh( void );
void log_config( void );
void SendToRing( unsigned char, char *, long );
int  Assoc( char * );
void Time_DtoS( double, char * );                    /* Time_DtoS.c  */


      /***********************************************************
       *              The main program starts here               *
       *                                                         *
       * Argument: configfile = Name of the configuration file   *
       ***********************************************************/

int main( int argc, char **argv )
{
   const long    InBufl = TRG_SIZE;
   char          trgmsg[TRG_SIZE];  /* One trigger message */
   MSG_LOGO      trglogo;           /* Requested logo(s) */
   MSG_LOGO      rcvdlogo;          /* Logo of retrieved msg */
   int           rc;                /* Return code */
   static char   line[40];
   long          rcvdlen;
   static time_t pulsetime = 0;     /* Time of last heartbeat */
   time_t        errTime;
   pid_t         mypid;
   FILE         *fp;
   int           exitstatus = 0;

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      printf( "Usage: evansassoc <configfile>\n" );
      return -1;
   }

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read configuration parameters
   *****************************/
   get_config( argv[1] );

/* Look up stuff in earthworm.h tables
   ***********************************/
   lookup_ewh();

/* Reinitialize logit to desired logging level
   *******************************************/
   logit_init( argv[1], 0, 256, LogSwitch );

/* Get our own Pid for restart purposes
 **************************************/
   mypid = getpid();
   if(mypid == -1)
   {
      logit("e", "evansassoc: Cannot get pid; exiting!\n" );
      return -1;
   }

/* Read file containing next valid eventid
   ***************************************/
   fp = fopen( EventIDfile, "r" );
   if( fp == (FILE *)NULL )
   {
      logit("e", "evansassoc: error opening EventIDfile <%s>; exiting!\n",
             EventIDfile );
      return -1;
   }
   if( fscanf( fp, "%ld", &NextEventID ) != 1 )
   {
      logit("e", "evansassoc: error reading EventIDfile <%s>; exiting!\n",
             EventIDfile );
      fclose( fp );
      return -1;
   }
   fclose( fp );
   NextEventID++;  /* skip an ID to insure no duplicates from last run*/

/* Log the configuration file parameters
   *************************************/
   log_config();

/* Attach to the transport ring (Must already exist)
   *************************************************/
   if ( OutKey != InKey )
   {
      tport_attach( &InRegion,  InKey );
      tport_attach( &OutRegion, OutKey );
   }
   else
   {   
      tport_attach( &InRegion, InKey );
      OutRegion = InRegion;
   }

/* Specify logo of trigger messages to get from the 
   transport ring; flush all old messages from ring
   *************************************************/
   trglogo.type   = TypeLpTrigSCNL;
   trglogo.mod    = GetThisModuleId;
   trglogo.instid = GetThisInst;
   while( tport_getmsg( &InRegion, &trglogo, 1, &rcvdlogo, &rcvdlen,
                         trgmsg, InBufl ) != GET_NONE );

/* Loop to read from the transport ring
   ************************************/
   while ( 1 )
   {
      if ( tport_getflag( &InRegion ) == TERMINATE ||
           tport_getflag( &InRegion ) == mypid )
      {
         logit( "t", "evansassoc: Termination requested; exiting.\n" );
         break;
      }

/* Get a trigger message from the transport InRegion;
   **************************************************/
      rc = tport_getmsg( &InRegion, &trglogo, 1, &rcvdlogo, &rcvdlen,
                         trgmsg, InBufl );

   /* Nothing to process; sleep & send a heartbeat to the transport ring
      ******************************************************************/
      if ( rc == GET_NONE )
      {
         sleep_ew( 1000 );

         time( &errTime );
         if ( HeartbeatInt > 0  &&
             (errTime - pulsetime) >= HeartbeatInt )
         {
            pulsetime = errTime;
            sprintf( line, "%ld %d\n", (long) pulsetime, (int) mypid );
            SendToRing( TypeHeartBeat, line, strlen(line) );
         }
         continue;
      }

   /* Check for other transport errors and log them
      *********************************************/
      else if ( rc == GET_TOOBIG ) /* try for another one */
      {
         logit("et","evansassoc: Retrieved msg[%ld] (i%u m%u t%u)"
               " overflows trgmsg[%ld]\n", rcvdlen, rcvdlogo.instid, 
                rcvdlogo.mod, rcvdlogo.type, InBufl );
         continue;
      }
      else if ( rc == GET_MISS ) /* got one, but: */
      {
         logit("et","evansassoc: missed msg(s) (i%u m%u t%u)\n",
                rcvdlogo.instid, rcvdlogo.mod, rcvdlogo.type );
      }
      else if ( rc == GET_NOTRACK ) /* got one, but: */
      {
         logit("et","evansassoc: got msg (i%u m%u t%u); cannot tell "
               "if any were missed (NTRACK_GET exceeded)\n",
                rcvdlogo.instid, rcvdlogo.mod, rcvdlogo.type );
      }

/* Send one trigger message to the associator (in doit.c)
   ******************************************************/
      if ( Assoc( trgmsg ) < 0 )
      {
         logit( "et", "evansassoc%d: Assoc failed!", MyModuleId );
         exitstatus = -1;
         break;
      }
   }

/* Detach from the transport ring
   ******************************/
   if ( OutKey != InKey )
   {
      tport_detach( &InRegion );
      tport_detach( &OutRegion );
   }
   else
   {
      tport_detach( &InRegion );
   }

   if( exitstatus ) 
   {
     logit( "et", "evansassoc: Exiting due to error condition %d!\n", 
             exitstatus );
   }

   return( exitstatus );
}


 /***********************************************************************
  *                              get_config()                           *
  *             Processes command file using kom.c functions.           *
  *                  Exits if any errors are encountered.               *
  ***********************************************************************/
#define ncommand 13         /* Number of required commands to process */

void get_config( char *configfile )
{
   char     init[ncommand]; /* Init flags, one byte for each command */
   int      nmiss;          /* Number of commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ ) init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
        logit("e", "evansassoc: Error opening configuration file <%s>; Exiting!\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
   *************************/
   while ( nfiles > 0 )            /* While there are command files open */
   {
        while ( k_rd() )           /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

         /* Ignore blank lines & comments
          *****************************/
            if ( !com )          continue;
            if ( com[0] == '#' ) continue;

         /* Open a nested configuration file
          ********************************/
            if( com[0] == '@' )
            {
               success = nfiles + 1;
               nfiles  = k_open( &com[1] );
               if ( nfiles != success ) {
                  logit("e", "evansassoc: Error opening configuration file <%s>; Exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

         /* Process anything else as a command
          **********************************/
            if ( k_its( "MyModuleId" ) )
            {
               str = k_str();
               if ( str )
                  if ( GetModId( str, &MyModuleId ) != 0 )
                  {
                     logit("e", "evansassoc: Invalid MyModuleId <%s> in <%s>; Exiting!\n",
                              str , configfile );
                     exit( -1 );
                  }
               init[0] = 1;
            }

            else if ( k_its( "GetTriggersFrom" ) )
            {
               if ( ( str = k_str() ) )
                  if ( GetInst( str, &GetThisInst ) != 0 )
                  {
                      logit("e", "evansassoc: Invalid GetTriggersFrom installation <%s>",
                               str );
                      logit("e", " in <%s>; Exiting!\n", configfile );
                      exit( -1 );
                  }

               if ( ( str = k_str() ) )
                  if ( GetModId( str, &GetThisModuleId ) != 0 )
                  {
                      logit("e", "evansassoc: Invalid GetTriggersFrom module <%s>", str );
                      logit("e", " in <%s>; Exiting!\n", configfile );
                      exit( -1 );
                  }
               init[1] = 1;
            }

            else if ( k_its( "InRing" ) )
            {
               if ( ( str=k_str() ) )
                  if ( (InKey = GetKey(str)) == -1 )
                  {
                     logit("e", "evansassoc: Invalid InRing name <%s>; Exiting!\n",
                              str );
                     exit( -1 );
                  }
               init[2] = 1;
            }

            else if ( k_its( "HeartbeatInt" ) )
               HeartbeatInt = k_int(), init[3] = 1;

            else if ( k_its( "TriggerTimeLimit" ) )
               TriggerTimeLimit = k_int(), init[4] = 1;

            else if ( k_its( "CriticalNu" ) )
               CriticalNu = k_int(), init[5] = 1;

            else if ( k_its( "CriticalMu" ) )
               CriticalMu = k_int(), init[6] = 1;

            else if ( k_its( "LogFile" ) )
               LogSwitch = k_int(), init[7] = 1;

            else if ( k_its( "EventIDfile" ) )
            {
               if ( ( str=k_str() ) )
               {
                  if ( strlen(str) > (size_t)NAME_LEN )
                  {
                     logit("e", "evansassoc: EventIDfile name <%s> too long "
                             "in <%s>; Exiting!\n", str, configfile );
                     exit( -1 );
                  }
                  strcpy( EventIDfile, str );
                  init[8] = 1;
               }
               else
               {
                  logit("e", "evansassoc: No filename in <EventIDfile> command "
                          "in <%s>; Exiting!\n", configfile );
                  exit( -1 );
               }
            }

            else if ( k_its( "PreEventTime" ) )
               PreEventTime = k_int(), init[9] = 1;

            else if ( k_its( "MinEventTime" ) )
               MinEventTime = k_int(), init[10] = 1;

            else if ( k_its( "MaxEventTime" ) )
               MaxEventTime = k_int(), init[11] = 1;

            else if ( k_its( "OutRing" ) )
            {
               if ( ( str=k_str() ) )
                  if ( (OutKey = GetKey(str)) == -1 )
                  {
                     logit("e", "evansassoc: Invalid OutRing name <%s>; Exiting!\n",
                              str );
                     exit( -1 );
                  }
               init[12] = 1;
            }

            else if ( k_its( "AddChannel" ) )  /* OPTIONAL */
            {
                int badarg = 0;   
                char *argname[] = {"","sta","chan","net","loc"};
                if( nExtraSCNL >= MaxExtraSCNL )
                {
                    size_t size;
                    MaxExtraSCNL += 10;
                    size     = MaxExtraSCNL * sizeof( SCNL_STRUCT );
                    ExtraSCNL = (SCNL_STRUCT *) realloc( ExtraSCNL, size );
                    if( ExtraSCNL == NULL )
                    {
                        logit("e",
                               "evansassoc: Error allocating %d bytes"
                               " for ExtraSCNL list; exiting!\n", size );
                        exit( -1 );
                    }
                }
                str=k_str();
                if( !str || strlen(str)>=(size_t)TRACE2_STA_LEN ) {
                    badarg = 1; goto EndChannel;
                }
                strcpy(ExtraSCNL[nExtraSCNL].sta,str);

                str=k_str();
                if( !str || strlen(str)>=(size_t)TRACE2_CHAN_LEN) {
                    badarg = 2; goto EndChannel;
                }
                strcpy(ExtraSCNL[nExtraSCNL].chan,str);

                str=k_str();
                if( !str || strlen(str)>=(size_t)TRACE2_NET_LEN) {
                    badarg = 3; goto EndChannel;
                }
                strcpy(ExtraSCNL[nExtraSCNL].net,str);

                str=k_str();
                if( !str || strlen(str)>=(size_t)TRACE2_LOC_LEN) {
                    badarg = 4; goto EndChannel;
                }
                strcpy(ExtraSCNL[nExtraSCNL].loc,str);


             EndChannel:
                if( badarg ) {
                   logit("e", 
                           "evansassoc: Argument %d (%s) bad in <AddChannel> "
                           "command (too long, missing, or invalid value):\n"
                           "   \"%s\"\n", badarg, argname[badarg], k_com() );
                   logit("e", "evansassoc: exiting!\n" );
                   free( ExtraSCNL );
                   exit( -1 );
                }
                nExtraSCNL++;
            }

         /* Unknown command
          *****************/
            else
            {
                logit("e", "evansassoc: <%s> unknown command in <%s>\n",
                        com, configfile );
                continue;
            }

         /* See if there were any errors processing the command
          *****************************************************/
            if( k_err() )
            {
               logit("e", "evansassoc: Bad <%s> command in <%s>; Exiting!\n",
                        com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
   ****************************************************************/
   nmiss = 0;
   for ( i = 0; i < ncommand; i++ )
      if( !init[i] ) nmiss++;
   if ( nmiss )
   {
       logit("e", "evansassoc: ERROR, no " );
       if ( !init[0] ) logit("e", "<MyModuleId> "       );
       if ( !init[1] ) logit("e", "<GetTriggersFrom> "  );
       if ( !init[2] ) logit("e", "<InRing> "           );
       if ( !init[3] ) logit("e", "<HeartbeatInt> "     );
       if ( !init[4] ) logit("e", "<TriggerTimeLimit> " );
       if ( !init[5] ) logit("e", "<CriticalNu> "       );
       if ( !init[6] ) logit("e", "<CriticalMu> "       );
       if ( !init[7] ) logit("e", "<LogFile> "          );
       if ( !init[8] ) logit("e", "<EventIDfile> "      );
       if ( !init[9] ) logit("e", "<PreEventTime> "     );
       if ( !init[10]) logit("e", "<MinEventTime> "     );
       if ( !init[11]) logit("e", "<MaxEventTime> "     );
       if ( !init[12]) logit("e", "<OutRing> "          );

       logit("e", "command(s) in <%s>; Exiting!\n", configfile );
       exit( -1 );
   }
   return;
}


   /************************************************************
    *                      lookup_ewh()                        *
    *     Look up important info from earthworm.h tables       *
    ************************************************************/

void lookup_ewh( void )
{
/* Find local installation id
   **************************/
   if ( GetLocalInst( &MyInstId ) != 0 )
   {
      printf( "evansassoc: Error getting local installation id; Exiting!\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
   {
      printf( "evansassoc: Invalid message type <TYPE_HEARTBEAT>; Exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_LPTRIG_SCNL", &TypeLpTrigSCNL ) != 0 )
   {
      printf( "evansassoc: Invalid message type <TYPE_LPTRIG_SCNL>; Exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRIGLIST_SCNL", &TypeTrigListSCNL ) != 0 )
   {
      printf( "evansassoc: Invalid message type <TYPE_TRIGLIST_SCNL>; Exiting!\n" );
      exit( -1 );
   }

   return;
}


   /************************************************************
    *                       log_config()                       *
    *          Log the configuration file parameters           *
    ************************************************************/

void log_config()
{
   char out[2];

   strcpy( out, "e" );
#ifdef _SOLARIS
   strcpy( out, "" );
#endif

   logit( out, "\nConfiguration file parameters:\n" );
   logit( out, "   Module id of this program:       %u\n",  MyModuleId );
   logit( out, "   Receive triggers from module id: %u\n",  GetThisModuleId );
   logit( out, "   Local installation id:           %u\n",  MyInstId );
   logit( out, "   Log to disk (0=no, 1=yes):       %d\n",  LogSwitch );
   logit( out, "   Heartbeat interval (seconds):    %d\n",  HeartbeatInt );
   logit( out, "   Get This Installation Id:        %u\n",  GetThisInst );
   logit( out, "   TriggerTimeLimit (seconds):      %d\n",  TriggerTimeLimit );
   logit( out, "   CriticalNu:                      %d\n",  CriticalNu );
   logit( out, "   CriticalMu:                      %d\n",  CriticalMu );
   logit( out, "   MinEventTime 'normal' (sec):     %d\n",  MinEventTime );
   logit( out, "   MaxEventTime 'big' (sec):        %d\n",  MaxEventTime );
   logit( out, "   PreEventTime (sec):              %d\n",  PreEventTime );
   logit( out, "   File containing next event ID:   %s\n",  EventIDfile );
   logit( out, "   Next valid event ID:             %ld\n", NextEventID );
   logit( out, "\n" );
   return;
}

   /************************************************************
    *                       GetEventID()                       *
    *  Returns the next valid eventid and updates the eventid  *
    *  file being used by this module                          *
    ************************************************************/
long GetEventID( void )
{
   long  id2return;
   FILE *fp;

   id2return = NextEventID++;

   fp = fopen( EventIDfile, "w" );
   if( fp == (FILE *)NULL )
   {
      logit("et", "evansassoc: error opening EventIDfile <%s>\n",
             EventIDfile );
   }
   else
   {
      fprintf( fp, "%ld", NextEventID );
      fclose( fp );
   }

   return( id2return );
}     


   /************************************************************
    *                       SendToRing()                       *
    *            Writes a message to the transport ring        *
    ************************************************************/
void SendToRing( unsigned char type, char *msg, long msglen )
{
   MSG_LOGO logo;
   int      rc;
   
   logo.type   = type;
   logo.mod    = MyModuleId;
   logo.instid = MyInstId;

   rc = tport_putmsg( &OutRegion, &logo, msglen, msg );

   if( rc != PUT_OK )
   {
      logit("et","evansassoc%d: Error sending msg type:%d to ring:%ld;", 
            (int) MyModuleId, (int) type, InKey );
      if     ( rc == PUT_NOTRACK ) logit(""," NTRACK_PUT exceeded.\n");
      else if( rc == PUT_TOOBIG )  logit(""," msg[%ld] too long.\n", msglen);
      else                         logit("","\n");
   } 

   return;
}

   /*********************************************************************
    *                         AppendExtraSCNL()                         *
    *   Append all Extra channels to the TYPE_TRIGLIST_SCNL message     *
    *   Returns: 0 if all went well,                                    *
    *            N the number of channels that were not included in msg *
    *              when msglen is too small for entire ExtraSCNL list   *
    *********************************************************************/

int AppendExtraSCNL( char *msg, int msglen, double ton, int duration )
{
   char ton_str[25];
   char line[256];
   int  nskip = 0;
   int  i;
  
   Time_DtoS( ton, ton_str );
   ton_str[21] = '\0';

   for( i=0; i<nExtraSCNL; i++ )
   {
      sprintf( line, " %s %s %s %s X 00000000 00:00:00.00 UTC save: %s   %d\n",
               ExtraSCNL[i].sta, ExtraSCNL[i].chan, ExtraSCNL[i].net,
               ExtraSCNL[i].loc, ton_str, duration );  

      if( ( strlen(msg) + strlen(line) ) >= (unsigned)msglen )  nskip++;
      else  strcat( msg, line );   /* only copy to msg if there's room, */
      logit("", "%s", line );      /* but always write it to log file   */
   }

   return( nskip );
}
