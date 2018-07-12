
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: reboot_mss_ew.c 3762 2010-01-02 00:39:51Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2010/01/02 00:39:51  kress
 *     bookeeping on gcf2ew.  reboot_mss and reboot_mss_ew now both work in linux.
 *
 *     Revision 1.8  2007/02/26 20:57:45  paulf
 *     fixed time_t issues for windows
 *
 *     Revision 1.7  2007/02/26 17:16:53  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.6  2004/06/25 18:27:27  dietz
 *     modified to work with TYPE_TRACEBUF2 and location code
 *
 *     Revision 1.5  2002/06/11 14:32:06  patton
 *     Made logit changes.
 *
 *     Revision 1.4  2002/03/19 23:18:38  kohler
 *     Program now exits if the "Logout" parameter is not set to
 *     0 or 1 in the config file.
 *
 *     Revision 1.3  2001/05/09 00:23:27  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or myPid.
 *
 *     Revision 1.2  2001/04/27 00:56:34  kohler
 *     Implemented option for MSS serial port logout.
 *
 *     Revision 1.1  2001/04/26 17:49:54  kohler
 *     Initial revision
 *
 *
 *
 */
       /****************************************************
        *                   reboot_mss_ew                  *
        *                                                  *
        *           Program to reboot an MSS100.           *
        *                                                  *
        *  If no data is received from a K2 for a while,   *
        *  we assume the MSS100 is hung.  Then, we         *
        *  attempt to reboot the MSS100.                   *
        ****************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <transport.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <trheadconv.h>
#include <kom.h>
#include <swap.h>
#include "reboot_mss_ew.h"

/* Function prototypes
   *******************/
int  GetConfig( char *, GPARM *, SCNL ** );
void LogConfig( GPARM *, SCNL * );
int  SpawnRebootMSS( char [], SCNL *, int );
int  TestChild( SCNL *, int * );

/* Global variables
   ****************/
static SCNL         *scnl;         /* Array of scnl parameters */
static TracePacket   TracePkt;
static char         *TraceBuf;     /* The tracebuf buffer */
static TRACE_HEADER *TraceHeader;  /* The tracebuf header */
static GPARM         Gparm;        /* Configuration file parameters */
pid_t                myPid;        /* for restarts by startstop */
static SHM_INFO      region;       /* Shared memory region */
static MSG_LOGO      errlogo;      /* Logo of outgoing errors */


int main( int argc, char *argv[] ) {
  MSG_LOGO getlogo[2];
  MSG_LOGO logo;
  MSG_LOGO hrtlogo;              /* Logo of outgoing heartbeats */
  long     gotsize;
  time_t   startTime;
  time_t   prevHeartTime;
  unsigned char inst_wildcard;
  unsigned char inst_local;
  unsigned char type_tracebuf;
  unsigned char type_tracebuf2;
  unsigned char type_heartbeat;
  unsigned char type_error;
  unsigned char mod_wildcard;
  unsigned char mod_reboot_mss_ew;
  unsigned char seq;

  /* Get command line arguments**************************/
  if ( argc != 2 ){
    printf( "Usage: reboot_mss_ew <configfile>\n" );
    return -1;
  }

  /* Initilize logit***************/
  logit_init( argv[1], 0, 256, 1 );

  /* Get parameters from the configuration file***************************/
  if ( GetConfig( argv[1], &Gparm, &scnl ) == -1 ){
    logit("et", "reboot_mss_ew: GetConfig() failed. Exiting.\n" );
    return -1;
  }

  /* Open the log file*****************/
  if ( strlen( Gparm.MyModName ) != 0 ){     /* use config file MyModuleId */
    if ( GetModId( Gparm.MyModName, &mod_reboot_mss_ew ) != 0 ){
      logit("et", "reboot_mss_ew: Error getting %s. Exiting.\n", 
	    Gparm.MyModName );
      return -1;
    }
  }
  else {                                     /* not configured, use default */
    if ( GetModId( "MOD_REBOOT_MSS_EW", &mod_reboot_mss_ew ) != 0 ) {
      logit("et", "reboot_mss_ew: Error getting MOD_REBOOT_MSS_EW. Exiting.\n" );
      return -1;
    }
  }
  /* Log the configuration parameters********************************/
  LogConfig( &Gparm, scnl );

  /* Sanity check************/
  if ( (Gparm.Logout != 0) && (Gparm.Logout != 1) ){
    logit( "et", "Error. Logout parameter must be 0 or 1. Exiting.\n" );
    return -1;
  }

  /* Initialize some parameters in the SCNL structures*********************/
  {
    int p;
    for ( p = 0; p < Gparm.nSCNL; p++ ){
      scnl[p].gapStart = time( 0 );      /* Current system time */
      scnl[p].reboot_active = 0;         /* No active reboots at startup */
    }
  }

  /* Get process ID for heartbeat messages*******************/
   myPid = getpid();
   if ( myPid == -1 ) {
     logit( "e","reboot_mss_ew: Cannot get pid. Exiting.\n" );
     return -1;
   }

   /* Attach to a transport ring**************************/
   tport_attach( &region, Gparm.InKey );

   /* Specify logos to get********************/
   if ( GetInst( "INST_WILDCARD", &inst_wildcard ) != 0 ) {
     logit( "e", "reboot_mss_ew: Error getting INST_WILDCARD. Exiting.\n" );
     return -1;
   }

   if ( GetLocalInst( &inst_local ) != 0 ){
     logit( "e", "reboot_mss_ew: Error getting MyInstId.\n" );
     return -1;
   }

   if ( GetType( "TYPE_TRACEBUF", &type_tracebuf ) != 0 ){
     logit( "e", "reboot_mss_ew: Error getting <TYPE_TRACEBUF>. Exiting.\n" );
     return -1;
   }

   if ( GetType( "TYPE_TRACEBUF2", &type_tracebuf2 ) != 0 ) {
     logit( "e", "reboot_mss_ew: Error getting <TYPE_TRACEBUF2>. Exiting.\n" );
     return -1;
   }

   if ( GetType( "TYPE_HEARTBEAT", &type_heartbeat ) != 0 ){
     logit( "e", "reboot_mss_ew: Error getting <TYPE_HEARTBEAT>. Exiting.\n" );
     return -1;
   }

   if ( GetType( "TYPE_ERROR", &type_error ) != 0 ) {
     logit( "e", "reboot_mss_ew: Error getting <TYPE_ERROR>. Exiting.\n" );
     return -1;
   }

   if ( GetModId( "MOD_WILDCARD", &mod_wildcard ) != 0 ) {
     logit( "e", "reboot_mss_ew: Error getting MOD_WILDCARD. Exiting.\n" );
     return -1;
   }

   /* Specify logos of incoming tracebufs and outgoing heartbeats*****/
   getlogo[0].instid = getlogo[1].instid = inst_wildcard;
   getlogo[0].mod    = getlogo[1].mod    = mod_wildcard;
   getlogo[0].type   = type_tracebuf2;
   getlogo[1].type   = type_tracebuf;
   
   hrtlogo.instid = inst_local;
   hrtlogo.mod    = mod_reboot_mss_ew;
   hrtlogo.type   = type_heartbeat;
   
   errlogo.instid = inst_local;
   errlogo.mod    = mod_reboot_mss_ew;
   errlogo.type   = type_error;
   
   /* Flush the transport ring on startup*****************/
   while (tport_copyfrom( &region, getlogo, (short)2, &logo, &gotsize,
                          TracePkt.msg, MAX_TRACEBUF_SIZ, &seq ) != GET_NONE);

   /* Get the time when we start reading messages
   *******************************************/
   time( &startTime );
   prevHeartTime = startTime;

   /* Master loop***********/
   /************************/
   while ( 1 ){
     int    flag;
     int    i;
     time_t now;
     
     sleep_ew( 1000 );
     
     /* See if the termination flag has been set**********/
     flag = tport_getflag( &region );
     if ( flag==TERMINATE || flag==myPid ){
       tport_detach( &region );
       logit( "t", "Termination flag detected. Exiting.\n" );
       return 0;
     }

     /* Send a heartbeat to the transport ring*****/
      now = time( 0 );
      if ( Gparm.HeartbeatInt > 0 ) {
	if ( (now - prevHeartTime) >= Gparm.HeartbeatInt ) {
	  int  lineLen;
	  char line[40];

	  prevHeartTime = now;

	  sprintf( line, "%ld %ld\n", (long) now, (long) myPid );
	  lineLen = strlen( line );

	  if ( tport_putmsg( &region, &hrtlogo, lineLen, line ) !=
	       PUT_OK ){
	    logit( "t", "Error sending heartbeat. Exiting." );
	    return -1;
	  }
	}
      }

      /* See if any reboot processes have completed. TestChild() returns:
	 2 if child completed and exitCode is set.
	 1 if child completed and exitCode not set.
	 0 if child process hasn't completed.
	 -1 if an error occured.
      ******/
      for ( i = 0; i < Gparm.nSCNL; i++ ){
	if ( scnl[i].reboot_active ){
	  int exitCode;
	  int rc = TestChild( &scnl[i], &exitCode );
	  
	  if ( rc == 1 ) {
	    if ( Gparm.Logout == 0 )
	      logit("t","%s Can't tell if reboot succeeded.",scnl[i].mss_ip);
	    else if ( Gparm.Logout == 1 )
	      logit("t","%s Can't tell if logout succeeded.",scnl[i].mss_ip);
	    scnl[i].reboot_active = 0;
	  }
	  if ( rc == 2 ) {
	    if ( exitCode == 0 ) {
	      if ( Gparm.Logout == 0 )
		logit( "t", "%s MSS100 reboot succeeded\n", scnl[i].mss_ip );
	      else if ( Gparm.Logout == 1 )
		logit( "t", "%s MSS100 logout succeeded\n", scnl[i].mss_ip );
	    }
	    else {
	      if ( Gparm.Logout == 0 )
		logit( "t", "%s MSS100 reboot failed\n", scnl[i].mss_ip );
	      else if ( Gparm.Logout == 1 )
		logit( "t", "%s MSS100 logout failed\n", scnl[i].mss_ip );
	    }
	    scnl[i].reboot_active = 0;
	  }
	}
      }
      /* Get all available tracebuf messages.
	 Update gapStart for the scnl's we are monitoring.
      *************************************************/
      while ( 1 ) {
	int res = tport_copyfrom( &region, getlogo, (short)2, &logo, &gotsize,
                                   TracePkt.msg, MAX_TRACEBUF_SIZ, &seq );

	if ( res == GET_NONE ) break;

	if ( res == GET_TOOBIG ){
	  logit( "t", "Retrieved message is too big (%d)\n", gotsize );
	  break;
	}

	if ( res == GET_NOTRACK ) logit( "t", "NTRACK_GET exceeded.\n" );

	if ( res == GET_MISS_LAPPED ) logit( "t", "GET_MISS_LAPPED error.\n" );

	/* If we're monitoring this SCNL, save the current time.
	   First make all packets look like TYPE_TRACEBUF2 pkts.
	*****************************************************/
	if( logo.type == type_tracebuf ) TrHeadConv( &TracePkt.trh );

	for ( i = 0; i < Gparm.nSCNL; i++ ) {
	  if ( !strcmp(scnl[i].sta,  TracePkt.trh2.sta)  &&
	       !strcmp(scnl[i].comp, TracePkt.trh2.chan) &&
	       !strcmp(scnl[i].net,  TracePkt.trh2.net)  && 
	       !strcmp(scnl[i].loc,  TracePkt.trh2.loc)     ) {
	    scnl[i].gapStart = (int) now;
	    break;
	  }
	}
      }

      /* See how much time has elapsed since
	 we saw data from each monitored SCNL
      ************************************/
      for ( i = 0; i < Gparm.nSCNL; i++ ){
	int GapSize = (int) (now - scnl[i].gapStart); 
	/* this could overflow if gap is huge and probably 
	   should be converted to an unsigned long */
	
	/* If the gap is too big and we aren't already rebooting,
	   invoke the reboot function****/
	if ( GapSize >= Gparm.RebootGap ) {
	  if ( scnl[i].reboot_active == 0 ) {
	    if (SpawnRebootMSS(Gparm.ProgName,&scnl[i],Gparm.Logout)<0){
	      if ( Gparm.Logout == 0 )
		logit( "t", "%s Error spawning process to reboot MSS100\n",
		       scnl[i].mss_ip );
	      else if ( Gparm.Logout == 1 )
		logit( "t", "%s Error spawning process to log out MSS100\n",
		       scnl[i].mss_ip );
	    }
	    else {
	      if ( Gparm.Logout == 0 )
		logit( "t", "%s Rebooting the MSS100\n", scnl[i].mss_ip );
	      else if ( Gparm.Logout == 1 )
		logit( "t", "%s Logging out the MSS100\n", scnl[i].mss_ip );
	      scnl[i].reboot_active = 1;
	    }
	  }
	  scnl[i].gapStart = now;
	}
      }
   }
}
