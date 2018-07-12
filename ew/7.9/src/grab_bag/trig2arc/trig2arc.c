
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: trig2arc.c,v 1.1 2010/01/01 00:00:00 jmsaurel Exp $
 *
 *    Revision history:
 *     $Log: trig2arc.c,v $
 *     Revision 1.1 2010/01/01 00:00:00 jmsaurel
 *     Initial revision
 *
 *
 */

#include "trig2arc.h"
#include "hyp_trig_utils.h"

/* Functions in this source file
 *******************************/
void trig2arc_config( char * );
void trig2arc_lookup( void );
void trig2arc_status( unsigned char, short, char * );
int  convert_trigmsg( char *, long *);
int  getNextLine (char**, char*);


int main( int argc, char **argv )
{
   char         *msgbuf;           /* buffer for msgs from ring     */
   time_t        timeNow;          /* current time                  */
   time_t        timeLastBeat;     /* time last heartbeat was sent  */
   long          recsize;          /* size of retrieved message     */
   MSG_LOGO      reclogo;          /* logo of retrieved message     */
   MSG_LOGO      putlogo;          /* logo to use putting message into ring */
   int           res, rc;
   unsigned char seq;

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, (DB_MAX_TRIG_BYTES + 256), 1 );

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        logit( "e", "Usage: trig2arc <configfile>\n" );
        exit( 0 );
   }

/* Read the configuration file(s)
 ********************************/
   trig2arc_config( argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   trig2arc_lookup();

/*  Set logit to LogSwitch read from configfile
 **********************************************/
   logit_init( argv[1], 0, 256, LogSwitch );
   logit( "" , "trig2arc: Read command file <%s>\n", argv[1] );

/* Check for different in/out rings if UseOriginalLogo is set
 ************************************************************/
   if( UseOriginalLogo  &&  (InRingKey==OutRingKey) ) 
   {
      logit ("e", "trig2arc: InRing and OutRing must be different when"
                  "UseOriginalLogo is non-zero; exiting!\n");
      free( GetLogo );
      exit( -1 );
   }

/* Get our own process ID for restart purposes
 *********************************************/
   if( (MyPid = getpid()) == -1 )
   {
      logit ("e", "trig2arc: Call to getpid failed. Exiting.\n");
      free( GetLogo );
      exit( -1 );
   }

/* Allocate the message input buffer 
 ***********************************/
  if ( !( msgbuf = (char *) malloc( DB_MAX_TRIG_BYTES+1 ) ) )
  {
      logit( "et", 
             "trig2arc: failed to allocate %d bytes"
             " for message buffer; exiting!\n", DB_MAX_TRIG_BYTES+1 );
      free( GetLogo );
      exit( -1 );
  }

/* Initialize outgoing logo
 **************************/
   putlogo.instid = InstId;
   putlogo.mod    = MyModId;

/* Attach to shared memory rings
 *******************************/
   tport_attach( &InRegion, InRingKey );
   logit( "", "trig2arc: Attached to public memory region: %ld\n",
          InRingKey );
   tport_attach( &OutRegion, OutRingKey );
   logit( "", "trig2arc: Attached to public memory region: %ld\n",
           OutRingKey );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartbeatInt - 1;

/* Flush the incoming transport ring on startup
 **********************************************/ 
   while( tport_copyfrom(&InRegion, GetLogo, nLogo,  &reclogo,
          &recsize, msgbuf, DB_MAX_TRIG_BYTES, &seq ) != GET_NONE );

/*----------------------- setup done; start main loop -------------------------*/

  while ( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid )
  {
     /* send trig2arc's heartbeat
      ***************************/
        if( HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= HeartbeatInt )
        {
            timeLastBeat = timeNow;
            trig2arc_status( TypeHeartBeat, 0, "" );
        }

     /* Get msg & check the return code from transport
      ************************************************/
        res = tport_copyfrom( &InRegion, GetLogo, nLogo, &reclogo, 
                              &recsize, msgbuf, DB_MAX_TRIG_BYTES, &seq );

        switch( res )
        {
        case GET_OK:      /* got a message, no errors or warnings         */
             break;

        case GET_NONE:    /* no messages of interest, check again later   */
             sleep_ew(100); /* milliseconds */
             continue;

        case GET_NOTRACK: /* got a msg, but can't tell if any were missed */
             sprintf( Text,
                     "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                      reclogo.instid, reclogo.mod, reclogo.type );
             trig2arc_status( TypeError, ERR_NOTRACK, Text );
             break;

        case GET_MISS_LAPPED:     /* got a msg, but also missed lots      */
             sprintf( Text,
                     "Missed msg(s) from logo (i%u m%u t%u)",
                      reclogo.instid, reclogo.mod, reclogo.type );
             trig2arc_status( TypeError, ERR_MISSLAP, Text );
             break;

        case GET_MISS_SEQGAP:     /* got a msg, but seq gap               */
             sprintf( Text,
                     "Saw sequence# gap for logo (i%u m%u t%u s%u)",
                      reclogo.instid, reclogo.mod, reclogo.type, seq );
             trig2arc_status( TypeError, ERR_MISSGAP, Text );
             break;

       case GET_TOOBIG:  /* next message was too big, resize buffer      */
             sprintf( Text,
                     "Retrieved msg[%ld] (i%u m%u t%u) too big for msgbuf[%d]",
                      recsize, reclogo.instid, reclogo.mod, reclogo.type,
                      DB_MAX_TRIG_BYTES );
             trig2arc_status( TypeError, ERR_TOOBIG, Text );
             continue;

       default:         /* Unknown result                                */
             sprintf( Text, "Unknown tport_copyfrom result:%d", res );
             trig2arc_status( TypeError, ERR_TOOBIG, Text );
             continue;
       }
       msgbuf[recsize] = '\0'; /* Null terminate for ease of printing */
       logit( "ot", "trig2arc : message received\n%s\n",msgbuf);

    /* Convert the message
     *********************/
       if( reclogo.type == TypeTrig ) 
       {
		rc = convert_trigmsg( msgbuf, &recsize );
		if( rc != EW_SUCCESS ) logit("o","convert_trigmsg failed to convert :\n%s", msgbuf);
       }
      
       if( rc != EW_SUCCESS ) continue;

       if( UseOriginalLogo ) {
          putlogo.instid = reclogo.instid;
          putlogo.mod    = reclogo.mod;
       }       
       putlogo.type = TypeHyp2000Arc;

       if( tport_putmsg( &OutRegion, &putlogo, recsize, msgbuf ) != PUT_OK )
       {
          logit("et","trig2arc: Error writing %d-byte msg to ring; "
                     "original logo (i%u m%u t%u)\n", recsize,
                      reclogo.instid, reclogo.mod, reclogo.type );
       }
       logit( "ot", "trig2arc : output message sent\n%s\n",msgbuf);
   }

/*-----------------------------end of main loop-------------------------------*/

/* free allocated memory */
   free( GetLogo );
   free( msgbuf  );

/* detach from shared memory */
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
           
/* write a termination msg to log file */
   logit( "t", "trig2arc: Termination requested; exiting!\n" );
   fflush( stdout );
   return( 0 );
}

/******************************************************************************
 *  trig2arc_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define ncommand 7         /* # of required commands you expect to process   */
void trig2arc_config( char *configfile )
{
   char  init[ncommand];   /* init flags, one byte for each required command */
   int   nmiss;            /* number of required commands that were missed   */
   char *com;
   char *str;
   int   nfiles;
   int   success;
   int   i;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
               "trig2arc: Error opening command file <%s>; exiting!\n",
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
                         "trig2arc: Error opening command file <%s>; exiting!\n",
                          &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                if( LogSwitch<0 || LogSwitch>2 ) {
                   logit( "e",
                          "trig2arc: Invalid <LogFile> value %d; "
                          "must = 0, 1 or 2; exiting!\n", LogSwitch );
                   exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("MyModuleId") ) {
                if( (str=k_str()) != NULL ) {
                   if( GetModId( str, &MyModId ) != 0 ) {
                      logit( "e",
                             "trig2arc: Invalid module name <%s> "
                             "in <MyModuleId> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("InRing") ) {
                if( (str=k_str()) != NULL ) {
                   if( ( InRingKey = GetKey(str) ) == -1 ) {
                      logit( "e",
                             "trig2arc: Invalid ring name <%s> "
                             "in <InRing> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("OutRing") ) {
                if( (str=k_str()) != NULL ) {
                   if( ( OutRingKey = GetKey(str) ) == -1 ) {
                      logit( "e",
                             "trig2arc: Invalid ring name <%s> "
                             "in <OutRing> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[3] = 1;
            }

  /*4*/     else if( k_its("HeartbeatInt") ) {
                HeartbeatInt = k_long();
                init[4] = 1;
            }


         /* Enter installation & module to get messages from
          **************************************************/
  /*5*/     else if( k_its("GetLogo") ) {
                MSG_LOGO *tlogo = NULL;
                tlogo = (MSG_LOGO *)realloc( GetLogo, (nLogo+1)*sizeof(MSG_LOGO) );
                if( tlogo == NULL )
                {
                   logit( "e", "trig2arc: GetLogo: error reallocing"
                           " %d bytes; exiting!\n",
                           (nLogo+2)*sizeof(MSG_LOGO) );
                   exit( -1 );
                }
                GetLogo = tlogo;

                if( (str=k_str()) != NULL ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e",
                              "trig2arc: Invalid installation name <%s>"
                              " in <GetLogo> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                   if( (str=k_str()) != NULL ) {
                      if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                          logit( "e",
                                 "trig2arc: Invalid module name <%s>"
                                 " in <GetLogo> cmd; exiting!\n", str );
                          exit( -1 );
                      }
                      if( GetType( "TYPE_TRIGLIST_SCNL", &GetLogo[nLogo].type ) != 0 ) {
                          logit( "e",
                                 "trig2arc: Invalid message type <TYPE_TRIGLIST_SCNL>" 
                                 "; exiting!\n" );
                          exit( -1 );
                      }
                   }
                }
                nLogo+=1;
                init[5] = 1;
            }

  /*6*/     else if( k_its("Debug") ) {
                Debug = k_int();
                init[6] = 1;
            }

  /*opt*/   else if( k_its("UseOriginalLogo") )
            {
                UseOriginalLogo = k_int();
            }

  /*opt*/   else if( k_its("UseLatitude") )
            {
                UseLatitude = k_val();
            }

  /*opt*/   else if( k_its("UseLongitude") )
            {
                UseLongitude = k_val();
            }

  /*opt*/   else if( k_its("UseDepth") )
            {
                UseDepth = k_val();
            }

         /* Unknown command
          *****************/
            else {
                logit( "e", "trig2arc: <%s> Unknown command in <%s>.\n",
                       com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                      "trig2arc: Bad <%s> command in <%s>; exiting!\n",
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
       logit( "e", "trig2arc: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "            );
       if ( !init[1] )  logit( "e", "<MyModuleId> "         );
       if ( !init[2] )  logit( "e", "<InRing> "             );
       if ( !init[3] )  logit( "e", "<OutRing> "            );
       if ( !init[4] )  logit( "e", "<HeartbeatInt> "       );
       if ( !init[5] )  logit( "e", "<GetLogo> "            );
       if ( !init[6] )  logit( "e", "<Debug> "              );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/******************************************************************************
 *  trig2arc_lookup( )   Look up important info from earthworm tables         *
 ******************************************************************************/

void trig2arc_lookup( void )
{

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      logit( "e",
             "trig2arc: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e",
             "trig2arc: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e",
             "trig2arc: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRIGLIST_SCNL", &TypeTrig ) != 0 ) {
      logit( "e",
             "trig2arc: Invalid message type <TYPE_TRIGLIST_SCNL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
      logit( "e",
             "trig2arc: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }

   return;
}


/******************************************************************************
 * trig2arc_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void trig2arc_status( unsigned char type, short ierr, char *note )
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

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "trig2arc: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","trig2arc:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","trig2arc:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

/******************************************************************************
 * convert_trigmsg() converts a trig message into a hypo2000 arc message      *
 ******************************************************************************/
int  convert_trigmsg( char *msg, long *msglen)
{
   char		*tmpbuf;
   int		outlen,start_of_msg;
   static char	terminators[] = " \t\n"; /* we accept space, newline, and tab as terminators */
   char		*nxttok;
   char		line[MAX_STR];
   char		*nextLine;
   char		*nextHypLine;
   char		trigger_date[18];
   STATRIG	station_trig;

	/* Allocate the message output buffer 
	 ************************************/
    if ( !( tmpbuf = (char *) malloc( DB_MAX_TRIG_BYTES+1 ) ) )
    {
	logit( "et", 
	"convert_trigmsg: failed to allocate %d bytes"
	" for message buffer; exiting!\n", DB_MAX_TRIG_BYTES+1 );
	return(EW_FAILURE);
    }

    trigger_date[0]='\0';
    nextLine = msg;
    nextHypLine = tmpbuf;
    start_of_msg = 1;
    outlen = 0;

	/* Loop over the message lines
	 *****************************/
    while(getNextLine(&nextLine,line) >=0)
    {
	if(Debug == 1)
		logit( "o", "convert_trigmsg loop over line :%s|\n",line);
	if (start_of_msg)
	{
		if(!(nxttok = strtok( line, terminators)))
			break;
		while ( (nxttok = strtok( (char*)NULL, terminators)) != NULL ) { /* over tokens on this line */
			if ( strcmp( nxttok, "DETECTED" ) != 0)
				continue;
			/* Find trigger time
			 *******************/
			nxttok = strtok( (char*)NULL, terminators); /* should be the Event start date */
			if (nxttok ==NULL) { /* oops - there was nothing after save: */
				logit("et", "convert_trigmsg: Bad syntax in trigger message."
					" Cant find Event date in:\n.%s.\n", line);
				return(0);
			}
			strcpy( trigger_date, nxttok ); 	/* put the date string */
    
			nxttok = strtok( (char*)NULL, terminators); /* sould be the Event start time-of-day */
			if (nxttok ==NULL) { /* oops - there was nothing after save: */
				logit("et", "convert_trigmsg: Bad syntax in trigger message."
					" Cant find Event time of day in:\n.%s.\n", line);
			return(0);
			}
			strncat( trigger_date, nxttok, 2 ); 	/* put the hour string */
			strncat( trigger_date, nxttok+3, 2 ); 	/* put the minute string */
			strncat( trigger_date+12, nxttok+6, 5 ); 	/* put the fractionnal second string */
			station_trig.Sta_time = julsec17(trigger_date);	/* Convert trigger time to julian date */
			nxttok = strtok( (char*)NULL, terminators); /* should be UTC */
			nxttok = strtok( (char*)NULL, terminators); /* should be EVENT */
			nxttok = strtok( (char*)NULL, terminators); /* should be ID: */
			nxttok = strtok( (char*)NULL, terminators); /* should be ID number */
			if (nxttok ==NULL) { /* oops - there was nothing after ID: */
				logit("et", "convert_trigmsg: Bad syntax in trigger message."
					" Cant find Event ID in:\n.%s.\n", line);
				return(0);
			}
			station_trig.Id = atoi(nxttok); 	/* put the ID string */
		}
		write_hyp_header(&station_trig,&nextHypLine);
		outlen = 168;
			/* We found the header of the message */
		getNextLine(&nextLine, line); /* step over the blank line */
		getNextLine(&nextLine, line); /* step over the column titles line */
		getNextLine(&nextLine, line); /* step over the silly dashes line */
		start_of_msg = 0;
	}
			/* Header found, so we loop over the station lines */
	else if ( read_statrig_line(line,&station_trig) )
	{
		write_hyp_staline(&station_trig,&nextHypLine);	/* writing hyp2000arc station line and its shadow */
		outlen = outlen + 116;
	}
    }
    write_hyp_term(&station_trig,&nextHypLine);		/* writing hyp2000arc termination line and its shadow */
    outlen = outlen + 146;
    if(Debug == 1)
	logit( "o", "convert_trigmsg output : \n%s|\n",tmpbuf);
    strncpy(msg,tmpbuf,outlen+1);
    *msglen = outlen + 1;
    free( tmpbuf );
    return(EW_SUCCESS);
}   

/**************************************************************************
 *    getNextLine(msg, line) moves the next line from 'msg' into 'line'   *
 *                           returns the number of characters in the line *
 *			     Returns negative if error.			  *
 **************************************************************************/
int getNextLine ( char** pNxtLine, char* line)
{
    int i;
    char* nxtLine;

    nxtLine=*pNxtLine;

    for (i =0; i< LINE_LEN; i++) {
	line[i] = *nxtLine++;
	if ( (int)line[i] == 0 ) {
	    return(-1); /*  Not good */
	}
	if (line[i] == '\n') goto normal;
    }
    logit("","getNextLine error: line too long \n");
    return(-1);
    
 normal:
    line[i+1]=0;
    *pNxtLine = nxtLine;
    return(i);
   
}
/* --------------------- end of getNextLine() ----------------------------- */
