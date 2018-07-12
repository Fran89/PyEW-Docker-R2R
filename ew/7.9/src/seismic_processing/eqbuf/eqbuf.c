
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqbuf.c 6298 2015-04-10 02:49:19Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2008/09/26 22:25:00  kress
 *     Fix numerous compile warnings and some tab-related fortran errors for linux compile
 *
 *     Revision 1.3  2002/06/05 15:42:59  patton
 *     Made logit changes.
 *
 *     Revision 1.2  2001/05/15 17:05:16  lucky
 *     Changed include of queue_max_size.h to mem_circ_queue.h because the
 *     underlying queueing routines have changed
 *
 *     Revision 1.1  2000/02/14 17:02:31  lucky
 *     Initial revision
 *
 *
 */

  /********************************************************************
   *                             eqbuf                                *
   *        Program to buffer messages flowing through a pipe.        *
   ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <mem_circ_queue.h>

#define EQBUF_VERSION  "1.5 2012.09.26"

/* MaxMsgSize was hard coded to 50000 in older versions of eqbuf.c, 
   which is defined the max size for input/output msgs
*/
static long    MaxMsgSize = (long)MAX_BYTES_PER_EQ;
static int     RingSize;         /* max messages in output circular buffer       */

QUEUE q;                         /* QUEUE is volatile */
void  eqbuf_config( char * );    /* read configuration file using kom.c */

/* Things to read from configuration file
 ****************************************/
unsigned char MyModId;          /* eqbuf's module id (same as eqproc's)   */
static   int  LogSwitch = 1;    /* 0 if no logging should be done to disk */
static   char NextProcess[100]; /* command to send output to              */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
unsigned char TypeError;
unsigned char TypeKill;

static char    *enMsg = NULL;
static char    *deMsg = NULL;

int main( int argc, char *argv[] )
{
   MSG_LOGO reclogo = {0};
   int      type = 0;
   char     *configFile;
   unsigned tid;
   int      rc;
   thr_ret  SendMsg( void * );

/* Check the program arguments
   ***************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: eqbuf <configfile>\nVersion: %s\n", EQBUF_VERSION);
      exit( -1 );
   }
   configFile = argv[1];

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 512, 1 );

/* Read the configuration file
   ***************************/
   eqbuf_config( configFile );    /* uses kom.c functions */

/* Look up message types in earthworm.d tables
   *******************************************/
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      fprintf( stderr,
	      "eqbuf: Invalid message type <TYPE_ERROR>; exitting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 )
   {
      fprintf( stderr,
	      "eqbuf: Invalid message type <TYPE_KILL>; exitting!\n" );
      exit( -1 );
   }

/* Initialize the queue
   ********************/
   initqueue ( &q, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1);           

/* Create mutex
   ************/
   /* alex 6/7/97: in Seattle, trying to fix sausage problem */
   /* used to make sure that both threads don't reach for the queue at once */
   CreateMutex_ew( );

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, 512, LogSwitch );

/* Spawn the child program
   ***********************/
   if ( pipe_init( NextProcess, (unsigned long)0 ) == -1 )
   {
      fprintf( stderr,
	      "eqbuf: error starting next process <%s>; exitting!\n",
	       NextProcess );
      exit( -1 );
   }
   logit( "e", "eqbuf: piping output to <%s>\n", NextProcess );

/* Start the SendMsg thread
   ************************/
   if ( StartThread( SendMsg, (unsigned)0, &tid ) == -1 )
   {
      fprintf( stderr, "eqbuf: Error starting the SendMsg thread\n" );
      exit( -1 );
   }
   logit( "et", "eqbuf: SendMsg thread started...\n" );

   if ( ( enMsg = (char *) malloc(MaxMsgSize+1) ) == (char *) NULL ) 
   {
      logit( "e", "eqbuf/main(): error allocating msg; exitting!\n" );
      exit( -1 );
   }
/* Get a message from the parent
   *****************************/
   while ( 1 )
   {
      memset( enMsg, 0, MaxMsgSize+1 );
      rc = pipe_get( enMsg, MaxMsgSize, &type );
      reclogo.type = (unsigned char) type;
      if ( rc == -1 )            /* msg on pipe too big for target... */
      {                          /* ...complain and skip it           */
	 logit( "et", "eqbuf pipe_get error; message too long!\n" );
	 continue;
      }
      if ( rc < 0  )
      {  /* pipe is closed */
         if ( reclogo.type != TypeKill )
         {  /* SendMsg thread is not already terminating program */
            logit( "et", "eqbuf: Input pipe closed; triggering exit\n" );
            reclogo.type = TypeKill;      /* queue Kill msg to exit program */
            RequestMutex();
            if ( enqueue( &q, "", 0, reclogo ) != 0 )
               logit( "et", "eqbuf: Enqueue error sending TypeKill\n" );
            ReleaseMutex_ew();
         }
         sleep_ew( 60000 );            /* wait for SendMsg thread to exit */
           /* SendMsg thread should call 'exit', so shouldn't reach here */
         logit( "et", "eqbuf: SendMsg thread didn't cause program exit\n" );
         exit ( -1 );
         return ( -1 );
      }

/* Put the message at the rear of the queue
   ****************************************/
      RequestMutex(); /* alex 6/7/97 */
      if ( enqueue( &q, enMsg, rc, reclogo ) != 0 )
      {
         logit( "et", "eqbuf enqueue error.\n" );
      }
      ReleaseMutex_ew();
   }
   exit ( 0 );
   return ( 0 );
}


/****************************************************************************
 * eqbuf_config() processes the configuration file using kom.c functions    *
 *                exits if any errors are encountered                       *
 ****************************************************************************/
void eqbuf_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 4;
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
	logit( "e",
		"eqbuf: Error opening command file <%s>; exitting!\n",
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
		  logit( "e",
			  "eqbuf: Error opening command file <%s>; exitting!\n",
			   &com[1] );
		  exit( -1 );
	       }
	       continue;
	    }

	/* Process anything else as a command
	 ************************************/
	 /* Numbered commands are required
	  ********************************/
   /*0*/    if( k_its( "LogFile" ) )
	    {
		LogSwitch = k_int();
		init[0] = 1;
	    }
   /*1*/    else if( k_its( "MyModuleId" ) )
	    {
		if ( ( str=k_str() ) ) {
		   if ( GetModId( str, &MyModId ) != 0 ) {
		      logit( "e",
			     "eqbuf: Invalid module name <%s>; exitting!\n",
			      str );
		      exit( -1 );
		   }
		}
		init[1] = 1;
	    }
    /*2*/   else if( k_its("PipeTo") )
	    {
		str = k_str();
		if(str) strcpy( NextProcess, str );
		init[2] = 1;
	    }
    /*3*/   else if( k_its("RingSize") ) {
		RingSize = k_long();
		init[3] = 1;
	    }
	    else
	    {
		logit( "e", "eqbuf: <%s> Unknown command in <%s>.\n",
			com, configfile );
		continue;
	    }

	/* See if there were any errors processing the command
	 *****************************************************/
	    if( k_err() ) {
	       logit( "e",
		       "eqbuf: Bad <%s> command in <%s>; exitting!\n",
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
       logit( "e", "eqbuf: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "    );
       if ( !init[1] )  logit( "e", "<MyModuleId> " );
       if ( !init[2] )  logit( "e", "<PipeTo> " );
       if ( !init[3] )  logit( "e", "<RingSize> " );
       logit( "e", "command(s) in <%s>; exitting!\n", configfile );
       exit( -1 );
    }
    return;
}


  /******************************************************************
   *                            SendMsg                             *
   *  Program to get a message from the queue and send it to the    *
   *  next program via a pipe.                                      *
   ******************************************************************/
thr_ret SendMsg( void *dummy )
{
   long     len = 0;
   MSG_LOGO reclogo = {0};
   int      ret;

   if ( ( deMsg = (char *) malloc(MaxMsgSize+1) ) == (char *) NULL ) 
   {
      logit( "e", "eqbuf/SendMsg(): error allocating msg; exitting!\n" );
      exit( -1 );
   }
/* Get a message from the front of the
   queue and send to the child process
   ***********************************/
   while ( 1 )
   {
         memset( deMsg, 0, MaxMsgSize+1 );
	 RequestMutex(); /* alex 6/7/97 */
	 ret=dequeue(  &q, deMsg, &len, &reclogo );
	 ReleaseMutex_ew(); /* alex 6/7/97 */
	 if ( ret == 0 )
	 {
/*          printf( "%s", deMsg ); */
	    if ( pipe_put( deMsg, reclogo.type ) == -1 )
	       logit( "et", "pipe_put error from SendMsg in eqbuf\n" );
	    if ( pipe_error() || reclogo.type == TypeKill )
	    {
               if ( pipe_error() )
                  logit( "t", "eqbuf: Output pipe error; exiting\n" );
               else
                  logit( "t", "eqbuf: termination requested; exiting\n" );
               sleep_ew( 500 );  /* give time for msg to get through pipe */
               pipe_close();
	       CloseMutex(); /* alex 6/7/97 */
	       if (enMsg) free(enMsg);
	       if (deMsg) free(deMsg);
	       exit( 0 );
	       return THR_NULL_RET;
	    }
	 }
	 else /* if ( rc == -1 ) */
	 {
	    sleep_ew( 100 );
	 }
   }
   return THR_NULL_RET; /* should never get here */
}

