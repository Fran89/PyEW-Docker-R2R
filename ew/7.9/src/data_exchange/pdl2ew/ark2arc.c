/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *
 */

/*
 * pdl2ew.c:
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <errno.h>
#include "pdl2ew.h"
#include <sqlite3.h>
#include <time.h>
#include <time_ew.h>

SHM_INFO  Region;             /* shared memory region to use for i/o    */
pid_t     myPid;              /* for restarts by startstop              */

static char *TroubleSubdir = "trouble";   /* subdir for problem files    */
static char *SaveSubdir = "save";         /* subdir for processed files  */

#define VERSION_STR "0.9.8 - 2015-11-17"

/* Things to read or derive from configuration file
 **************************************************/
static char      RingName[20];        /* name of transport ring for i/o    */
static char      MyModName[50];       /* speak as this module name/id      */
static int       LogSwitch;           /* 0 if no logfile should be written */
static int       HeartBeatInterval;   /* seconds betweeen beats to statmgr */
static char      GetFromDir[NAM_LEN]; /* directory to monitor for data     */
static char      SaveToDir[NAM_LEN]; /* directory to write editied messages to     */
static unsigned  CheckPeriod;         /* secs between looking for new files*/
static int	 OpenTries = 5;
static int	 OpenWait = 3;
static int       SaveDataFiles;       /* if non-zero, move to SaveSubdir,  */ 
                                      /*           0, remove files         */
static int       LogHeartBeatFile;    /* if non-zero, write contents of    */ 
                                      /*  each heartbeat file to daily log */
static int       LogOutgoingMsg = 0;  /* if non-zero, write each outgoing  */
                                      /*  msg to the daily log file        */
//static int       SMFileFormat;        /* format of strong motion file      */

char      	 NetworkName[10];     /* Network code to use by default    */
int	         Debug;               /* non-zero -> debug logging         */

char      	 authority[10];       /* Network Authority code to use     */
static char      DBPath[NAM_LEN]; /* path to database file     */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
       unsigned char TypeError;
       unsigned char TypePage;

char Text[TEXT_LEN];         /* string for log/error messages      */
char ProgName[NAM_LEN];      /* program name for logging purposes  */
#define LOGIT_LEN TEXT_LEN*2
#define secsInWeek  604800.0

char* db_path;

long      eventID;

int callback1(void* notUsed, int argc, char** argv, char** azColName  ) {
    notUsed = 0;
    eventID = strtol( argv[0], NULL, 10 );
    return 0;
}

/******************************************************************************
 *  config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define ncommand 14
void config( char *configfile )
{
   char     init[ncommand]; /* init flags, one byte for each required command */
   int      nmiss;          /* number of required commands that were missed   */
   char    *com;
   char    *str;
   char     processor[20];
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;

	strcpy (NetworkName, "--");
	strcpy (authority, "--");

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
        logit("e","%s: Error opening command file <%s>; exiting!\n", 
               ProgName, configfile );
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
                  logit("e","%s: Error opening command file <%s>; exiting!\n",
                         ProgName, &com[1] );
                  exit( -1 );
               }
               continue;
            }
            strcpy( processor, "config" );

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("GetFromDir") ) {
                str = k_str();
                if(str) strncpy( GetFromDir, str, NAM_LEN );
                init[3] = 1;
            }
  /*4*/     else if( k_its("SaveToDir") ) {
                str = k_str();
                if(str) strncpy( SaveToDir, str, NAM_LEN );
                init[4] = 1;
            }
  /*5*/     else if( k_its("Debug") ) {
                Debug = k_int();
                init[5] = 1;
            }
  /*6*/     else if( k_its("OpenTries") ) {
                OpenTries = k_int();
                init[6] = 1;
            }
  /*7*/     else if( k_its("OpenWait") ) {
                OpenWait = k_int();
                init[7] = 1;
            }
  /*8*/     else if( k_its("SaveDataFiles") ) {
                SaveDataFiles = k_int();
                init[8] = 1;
            }

  /*9*/    else if( k_its("HeartBeatInterval") ) {
                HeartBeatInterval = k_int();
                init[9] = 1;
            }
  /*10*/    else if( k_its("LogHeartBeatFile") ) {
                LogHeartBeatFile = k_int();
                init[10] = 1;
            }
  /*11*/   else if( k_its("LogOutgoingMsg") ) {
                LogOutgoingMsg = k_int();
                init[11] = 1;
            }
  /*12*/   else if( k_its("DatabasePath") ) {
                str = k_str();
                if(str) strncpy( DBPath, str, NAM_LEN );
                init[12] = 1;
            }
  /*13*/     else if( k_its("CheckPeriod") ) {
                CheckPeriod = k_int();
                init[13] = 1;
            }
  /*opt*/   else if( k_its("NetworkName") ) {
                str = k_str();
                if(str) strncpy( NetworkName, str, 10 );
            }
 /*opt*/    else if( k_its("Authority") ) {
                str = k_str();
                str[2] = 0;
                if(str) strcpy( authority, str );
            }

         /* Unknown command
          *****************/ 
            else {
               logit( "e", "%s: <%s> Unknown command in <%s>.\n", 
                     ProgName, com, configfile );
               continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit( "e", "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
                      ProgName, com, processor, configfile );
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
       logit("e", "%s: ERROR, no ", ProgName );
       if ( !init[0] )  logit("e", "<LogFile> "               );
       if ( !init[1] )  logit("e", "<MyModuleId> "            );
       if ( !init[2] )  logit("e", "<RingName> "              );
       if ( !init[3] )  logit("e", "<GetFromDir> "            );
       if ( !init[4] )  logit("e", "<SaveToDir> "             );
       if ( !init[5] )  logit("e", "<Debug> "                 );
       if ( !init[6] )  logit("e", "<OpenTries> "             );
       if ( !init[7] )  logit("e", "<OpenWait> "              );
       if ( !init[8] )  logit("e", "<SaveDataFiles> "         );
       if ( !init[9])   logit("e", "<HeartBeatInterval> "     );
       if ( !init[10])  logit("e", "<LogHeartBeatFile> "      );
       if ( !init[11])  logit("e", "<LogOutgoingMsg> "        );
       if ( !init[12])  logit("e", "<DatabasePath> "          );
       if ( !init[13])  logit("e", "<CheckPeriod> "           );
       logit("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/******************************************************************************
 *  lookup( )   Look up important info from earthworm.h tables        *
 ******************************************************************************/
void lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        logit( "e", "%s:  Invalid ring name <%s>; exiting!\n",
               ProgName, RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      logit( "e", "%s: error getting local installation id; exiting!\n",
             ProgName );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      logit( "e", "%s: Invalid module name <%s>; exiting!\n", 
             ProgName, MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e", "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n",
             ProgName );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e", "%s: Invalid message type <TYPE_ERROR>; exiting!\n",
             ProgName );
      exit( -1 );
   }
   if ( GetType( "TYPE_PAGE", &TypePage ) != 0 ) {
      logit( "e", "%s: Invalid message type <TYPE_PAGE>; exiting!\n",
             ProgName );
      exit( -1 );
   }
   return;
} 

/******************************************************************************
 * status() builds a heartbeat or error message & puts it into        *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void status( unsigned char type, short ierr, char *note )
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
        sprintf( msg, "%ld %ld\n", (long) t, (long) myPid);
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "%s: %s\n", ProgName, note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */     

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","%s:  Error sending heartbeat.\n", ProgName );
        }
        else if( type == TypeError ) {
           logit("et","%s:  Error sending error:%d.\n", ProgName, ierr );
        }
   }
   return;
}



/* File handling stuff
**********************/
int main( int argc, char **argv )
{
   int       result;
//   int 	     read_error = 0;
   int	     i;
   char      fname[100];
   char      fnew[125];
   char     *c;
   FILE     *fp;
   time_t    tnextbeat;    /* next time for local heartbeat  */
   time_t    tnow;         /* current time */
   int       flag;         /* transport flag value */
   ARCDATA   arc_msg;
   int       rc;

/* Check command line arguments 
 ******************************/
   if ( argc != 2 )
   {
        //fprintf( stderr, "Usage: %s <absolute-path-to-source-directory> <absolute-path-to-dest-directory> <absolute-path-to-db>\n", argv[0] );
        fprintf( stderr, "Usage: %s <configfile>>\n", argv[0] );
        fprintf(stderr, "Version: %s\n", VERSION_STR );
        return EW_FAILURE;
   }
   strcpy( ProgName, argv[1] );
   c = strchr( ProgName, '.' );
   if( c ) *c = '\0';

/* Initialize name of log-file & open it 
 ***************************************/
   logit_init( argv[1], 0, LOGIT_LEN, 1 );   

/* Read the configuration file(s)
 ********************************/
   config(argv[1]);
   logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );
    logit("t", "%s: version %s\n", ProgName, VERSION_STR);

/* Lookup important information from earthworm.d
***********************************************/
    lookup();


/* Set logit to LogSwitch read from configfile
*********************************************/
    logit_init(argv[1], 0, LOGIT_LEN, LogSwitch);


/* Get process ID for heartbeat messages 
 ***************************************/
   myPid = getpid();
   if( myPid == -1 )
   {
     logit("e","%s: Cannot get pid; exiting!\n", ProgName);
     return( -1 );
   }

/* Change to the directory with the input files
 ***********************************************/
   if( chdir_ew( GetFromDir ) == -1 )
   {
      logit( "e", "%s: GetFromDir directory <%s> not found; "
                 "exiting!\n", ProgName, GetFromDir );
      return( -1 );
   }
   if(Debug)logit("et","%s: changed to directory <%s>\n", ProgName,GetFromDir);

/* Make sure trouble subdirectory exists
 ***************************************/
   if( CreateDir( TroubleSubdir ) != EW_SUCCESS ) {
      logit( "e", "%s: trouble creating trouble directory: %s/%s\n",
              ProgName, GetFromDir, TroubleSubdir ); 
      return( -1 );
   }

/* Make sure save subdirectory exists (if it will be used)
 *********************************************************/
   if( SaveDataFiles ) {
      if( CreateDir( SaveSubdir ) != EW_SUCCESS ) {
         logit( "e", "%s: trouble creating save directory: %s/%s\n",
                ProgName, GetFromDir, SaveSubdir ); 
      return( -1 );
      }
   }

/* Attach to Output shared memory ring 
 *************************************/
   tport_attach( &Region, RingKey );
   logit( "", "%s: Attached to public memory region %s: %d\n", 
          ProgName, RingName, RingKey );

/* Force local heartbeat first time thru main loop
   but give peers the full interval before we expect a file
 **********************************************************/
   tnextbeat  = time(NULL) - 1;


/* Access the DB
 ********************************/
   
   if (SQLITE_OK != (rc = sqlite3_initialize()))
	{
		logit("et","%s: Failed to initialize sqlite3 library: %d\n", ProgName, rc);
		return 1;
	}
   db_path = DBPath;
   
    sqlite3 *db;
    char *err_msg = 0;
    
    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        logit("et", "%s: Cannot open database @ %s: %s\n", ProgName, db_path, sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return -1;
    }
    char evt_sql[200], evtid[20];
	char *sql = "CREATE TABLE Events(PdlID TEXT, TimeStamp TEXT);";          
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    double nextPurge = time(NULL) + secsInWeek;
    
    int ins_count = 0, upd_count = 0, del_count = 0, op_count = 0;
/****************  top of working loop ********************************/
   while(1)
   {

     /* Check on heartbeats
      *********************/
        tnow = time(NULL);
        if( tnow >= tnextbeat ) {  /* time to beat local heart */
           status( TypeHeartBeat, 0, "" );
           tnextbeat = tnow + HeartBeatInterval;
        }

     /* See if termination has been requested 
      ****************************************/
        flag = tport_getflag( &Region );
 	if( flag == TERMINATE  ||  flag == myPid ) 
        {
           logit( "t", "%s: Termination requested; exiting!\n", ProgName );
           break;
        }

	result = GetFileName( fname );
	if( result == 1 ) {  /* No files found; wait for one to appear */
	        if ( time(NULL) > nextPurge ) {
	            logit( "t", "Purging week-old mappings\n" );
                char nextPurgeStr[30];
                datestr23( nextPurge, nextPurgeStr, 30 );
                nextPurgeStr[4] = nextPurgeStr[7] = '-';
                //FILES THAT NEED DELETING?
                sprintf( evt_sql, "DELETE FROM Events WHERE TimeStamp < '%s';", nextPurgeStr );
                if ( db == NULL ) {
                    logit( "et", "Re-opening DB\n" );
                    rc = sqlite3_open(db_path, &db);
                    if (rc != SQLITE_OK) {
                        logit("et", "Cannot re-open database: %s\n", sqlite3_errmsg(db));
                        sqlite3_close(db);
                        return 1;
                    }
                }
                rc = sqlite3_exec(db, evt_sql, 0, 0, &err_msg);  
                del_count++;
                op_count++;  
                if (rc != SQLITE_OK ) {
                    logit("et", "DELETE #%d failed\n", del_count);   
                    logit("et", "SQL DELETE error %d: %s (%s)\n", rc, err_msg, evt_sql);
                } 
                nextPurge = time(NULL) + secsInWeek;
	        }
           sleep_ew( CheckPeriod*1000 ); 
	   continue;
	}

    /* Skip non-ark files */
    i = strlen(fname);
    if ( i < 4 || strcmp( fname+i-4, ".ark" ) ) {
        logit( "t", "Ignoring/deleting %s\n", fname );
           sprintf(fnew,"%s/%s",TroubleSubdir,fname );
           if( rename_ew( fname, fnew ) != 0 ) {
             logit( "e", " error moving file to ./%s ; exiting!\n", 
                     fnew );
             break;
           } else {
             logit( "e", " moved to ./%s\n", fnew );
           }        continue;
    }
        
     /* Open the file.
      * We open for updating (even though we only want to read it), 
      * as that will hopefully get us an exclusive open. 
      * We don't ever want to look at a file that's being written to. 
      ***************************************************************/
        fp = NULL;
	for( i=0; i<OpenTries; i++ ) {
           fp = fopen( fname, "rb+" );
           if( fp != NULL ) break;
           sleep_ew( OpenWait );
	}
        if( fp == NULL ) { /* failed to open file! */
	   logit( "et","%s: Error: Could not open %s after %d*%d msec.",
                  ProgName, fname, OpenTries, OpenWait);
           result = -1;
           goto ProcessedFile;
        }
	if( i>0 ) {
           logit("t","Warning: %d attempts required to open file %s\n",
                  i+1, fname);
        }

    fread( (void*)&arc_msg, sizeof(arc_msg), 1, fp );
    
    strncpy( evtid, arc_msg.pad_eventid, 15 );
    evtid[15] = 0;
    eventID = -1;

    sprintf( evt_sql, "SELECT ROWID FROM Events WHERE PdlID = '%s'", evtid );
    if ( db == NULL ) {
        logit( "et", "Re-opening DB\n" );
        rc = sqlite3_open(db_path, &db);
        if (rc != SQLITE_OK) {
            logit("et", "Cannot re-open database: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
        }
    }
    rc = sqlite3_exec(db, evt_sql, callback1, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        logit("et", "SQL SELECT error %d: %s (%s)\n", rc, err_msg, evt_sql);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return 1;
    } 
    
    if ( eventID == -1 ) {
        logit( "t", "Inserting '%s'\n", evtid );
        sprintf( evt_sql, "INSERT INTO Events(PdlID,TimeStamp) VALUES('%s',datetime('now'));", evtid );
        rc = sqlite3_exec(db, evt_sql, 0, 0, &err_msg);
        ins_count++; op_count++;
        if (rc != SQLITE_OK ) {     
            logit("et", "INSERT #%d failed\n", ins_count);   
            logit("et", "SQL INSERT error %d: %s (%s)\n", rc, err_msg, evt_sql);
            sqlite3_free(err_msg);        
            sqlite3_close(db);
            
            rc = sqlite3_open(db_path, &db);
            if (rc != SQLITE_OK) {
                logit("et", "Cannot re-open database: %s\n", sqlite3_errmsg(db));
                sqlite3_close(db);
                return 1;
            }
            sleep_ew( 1 );
            rc = sqlite3_exec(db, evt_sql, 0, 0, &err_msg);
            if (rc != SQLITE_OK ) {        
                logit("et", "SQL INSERT(2) error %d: %s\n", rc, err_msg);
                sqlite3_free(err_msg);        
                sqlite3_close(db);
                return 1;
            } 
        }
        eventID = sqlite3_last_insert_rowid(db);
        fprintf( stderr, "New mapping: '%s' -> '%010ld'\n", evtid, eventID );        
    } else {
        logit( "t", "Updating '%s': %ld\n", evtid, eventID );
        sprintf( evt_sql, "UPDATE Events SET TimeStamp=date('now') WHERE PdlID='%s';", evtid );
        rc = sqlite3_exec(db, evt_sql, 0, 0, &err_msg);
        upd_count++; op_count++;
        if (rc != SQLITE_OK ) {
            logit("et", "UPDATE #%d (op #%d) failed\n", upd_count, op_count);   
            logit("et", "SQL UPDATE error %d: %s (%s)\n", rc, err_msg, evt_sql);
            sqlite3_free(err_msg);        
            sqlite3_close(db);
            return 1;
        } 
    }
    
    sprintf( arc_msg.eventID, "%010ld", eventID );
    arc_msg.pad5[0] = ' ';    
    
  ProcessedFile:
    if ( result >= 0 ) {
            logit( "t", "Saving '%s': %ld\n", evtid, eventID );

        char path[300];
        sprintf( path, "%s/__________.arc", SaveToDir );
        char *p2 = path+strlen(SaveToDir)+1;
        for ( i=0; i<sizeof(arc_msg.eventID); i++ )
            p2[i] = arc_msg.eventID[i];
        /* Add shadow line */
        memset( arc_msg.pad6, ' ', sizeof(arc_msg.pad6) );
        arc_msg.break1 = arc_msg.break2 = '\n';

	      if( remove( fname ) != 0 ) {
                 logit("e","error deleting file\n");
              } else  {
                 logit("","deleted file.\n");
              }
        fp = fopen( path, "w" );
        fwrite( (void*)&arc_msg, &arc_msg.break2 - arc_msg.originTime + 1, 1, fp );
        fclose( fp );
    } else { 
           logit("e","\n");
           sprintf( Text,"Trouble processing: %s ;", fname );
           sprintf(fnew,"%s/%s",TroubleSubdir,fname );
           if( rename_ew( fname, fnew ) != 0 ) {
             logit( "e", " error moving file to ./%s ; exiting!\n", 
                     fnew );
             break;
           } else {
             logit( "e", " moved to ./%s\n", fnew );
           }
        } 
        
    if ( ins_count >= 100 ) {
        logit( "et", "Closing DB\n" );
        sqlite3_close(db);
        db = NULL;
        ins_count = 0;
    }

   } /* end of while */

/************************ end of working loop ****************************/
	
/* detach from shared memory */
   tport_detach( &Region ); 

/* write a termination msg to log file */
   fflush( stdout );
   if ( db != NULL )
        sqlite3_close( db );
   sqlite3_shutdown();
   return( 0 );

}  
/************************* end of main ***********************************/
